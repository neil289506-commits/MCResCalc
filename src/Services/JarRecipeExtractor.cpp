#include "JarRecipeExtractor.h"

// NOTE: QuaZip 的 include 路徑依你安裝方式可能不同（vcpkg 的 "quazip" port
// 通常是 <quazip/quazip.h>；如果你裝的是 QuaZip-Qt6 這個 fork，可能要改成
// <QuaZip-Qt6/quazip.h> 或 <quazip1-qt6/quazip.h>，用 vcpkg list 確認一下 port 名稱，
// 編譯器找不到檔案的話優先檢查這裡。
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <algorithm>

using Models::IngredientOption;
using Models::RecipeDefinition;
using Models::RecipeKind;

namespace Services {

namespace {

QString normalizeId(QString id)
{
    if (!id.contains(':'))
        id = QStringLiteral("minecraft:") + id;
    return id;
}

/// <summary>舊式 {"item": "..."} / {"tag": "..."} / {"id": "..."} 物件寫法。</summary>
IngredientOption appendObjectIngredient(const QJsonObject &obj, IngredientOption opt)
{
    if (obj.contains("tag") && obj.value("tag").isString())
    {
        opt.isTag = true;
        opt.tagId = normalizeId(obj.value("tag").toString());
    }
    else if (obj.contains("item") && obj.value("item").isString())
    {
        opt.candidateItems.append(normalizeId(obj.value("item").toString()));
    }
    else if (obj.contains("id") && obj.value("id").isString())
    {
        opt.candidateItems.append(normalizeId(obj.value("id").toString()));
    }
    return opt;
}

/// <summary>
/// 解析一個材料格的值，支援目前已知的所有寫法：
///   "minecraft:stick"                    純字串
///   "#minecraft:planks"                  字串標籤
///   ["minecraft:a", "minecraft:b"]       陣列（OR 清單）
///   {"item": "minecraft:x"} / {"tag": "..."} / {"id": "..."}   舊式物件寫法
/// </summary>
std::optional<IngredientOption> parseIngredientValue(const QJsonValue &val, const QString &role = QString())
{
    if (val.isString())
    {
        const QString s = val.toString();
        IngredientOption opt;
        opt.role = role;
        if (s.startsWith(QLatin1Char('#')))
        {
            opt.isTag = true;
            opt.tagId = normalizeId(s.mid(1));
        }
        else
        {
            opt.candidateItems.append(normalizeId(s));
        }
        return opt;
    }

    if (val.isArray())
    {
        IngredientOption opt;
        opt.role = role;
        for (const QJsonValue &item : val.toArray())
        {
            if (item.isString())
            {
                const QString s = item.toString();
                if (s.startsWith(QLatin1Char('#')))
                {
                    opt.isTag = true;
                    opt.tagId = normalizeId(s.mid(1));
                }
                else
                {
                    opt.candidateItems.append(normalizeId(s));
                }
            }
            else if (item.isObject())
            {
                opt = appendObjectIngredient(item.toObject(), opt);
            }
        }
        if (opt.candidateItems.isEmpty() && !opt.isTag) return std::nullopt;
        return opt;
    }

    if (val.isObject())
    {
        IngredientOption opt;
        opt.role = role;
        opt = appendObjectIngredient(val.toObject(), opt);
        if (opt.candidateItems.isEmpty() && !opt.isTag) return std::nullopt;
        return opt;
    }

    return std::nullopt;
}

/// <summary>
/// "result" 欄位在 1.20.5 前後改過格式：
///   1.20.5 之前： {"item": "minecraft:x", "count": n}，或極舊版本直接是字串
///   1.20.5 之後： {"id": "minecraft:x", "count": n}
/// 這裡兩種都接受。
/// </summary>
bool tryParseResult(const QJsonObject &root, QString &resultItem, int &resultCount)
{
    resultItem.clear();
    resultCount = 1;
    if (!root.contains("result")) return false;

    const QJsonValue resVal = root.value("result");

    if (resVal.isString())
    {
        resultItem = resVal.toString();
        return !resultItem.isEmpty();
    }

    if (resVal.isObject())
    {
        const QJsonObject obj = resVal.toObject();
        if (obj.contains("id") && obj.value("id").isString())
            resultItem = obj.value("id").toString();
        else if (obj.contains("item") && obj.value("item").isString())
            resultItem = obj.value("item").toString();

        if (obj.contains("count") && obj.value("count").isDouble())
            resultCount = std::max(1, obj.value("count").toInt());

        return !resultItem.isEmpty();
    }

    return false;
}

} // namespace

ExtractedGameData JarRecipeExtractor::extract(const QString &jarPath)
{
    ExtractedGameData data;

    QuaZip zip(jarPath);
    if (!zip.open(QuaZip::mdUnzip))
        return data;

    // 1.21 之前資料夾是複數（recipes / items），1.21 開始改成單數（recipe / item）。
    // 這裡用 "s?" 讓兩種命名都吃得到，不用去猜每個版本的確切切換點。
    static const QRegularExpression recipeRe(QStringLiteral(R"(^data/([^/]+)/recipes?/(.+)\.json$)"),
                                               QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tagRe(QStringLiteral(R"(^data/([^/]+)/tags/items?/(.+)\.json$)"),
                                            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression langRe(QStringLiteral(R"(^assets/minecraft/lang/(en_us|zh_tw)\.json$)"),
                                             QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression modelRe(QStringLiteral(R"(^assets/([^/]+)/models/item/(.+)\.json$)"),
                                              QRegularExpression::CaseInsensitiveOption);

    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    {
        const QString name = zip.getCurrentFileName();

        auto readJson = [&]() -> QJsonDocument {
            QuaZipFile file(&zip);
            if (!file.open(QIODevice::ReadOnly)) return {};
            const QByteArray bytes = file.readAll();
            file.close();
            return QJsonDocument::fromJson(bytes);
        };

        QRegularExpressionMatch m;

        // ---- 配方 ----
        if ((m = recipeRe.match(name)).hasMatch())
        {
            const QString ns = m.captured(1);
            const QString path = m.captured(2);
            const QString id = ns + ":" + path;

            const QJsonDocument doc = readJson();
            if (!doc.isObject()) { data.skippedUnknownRecipeCount++; continue; }
            const QJsonObject root = doc.object();

            if (!root.contains("type")) continue;
            const QString typeStr = root.value("type").toString();
            const QString shortType = typeStr.contains(':') ? typeStr.mid(typeStr.indexOf(':') + 1) : typeStr;

            RecipeKind kind = RecipeKind::Unknown;
            if (shortType == QStringLiteral("crafting_shaped")) kind = RecipeKind::CraftingShaped;
            else if (shortType == QStringLiteral("crafting_shapeless")) kind = RecipeKind::CraftingShapeless;
            else if (shortType == QStringLiteral("smelting")) kind = RecipeKind::Smelting;
            else if (shortType == QStringLiteral("blasting")) kind = RecipeKind::Blasting;
            else if (shortType == QStringLiteral("smoking")) kind = RecipeKind::Smoking;
            else if (shortType == QStringLiteral("campfire_cooking")) kind = RecipeKind::CampfireCooking;
            else if (shortType == QStringLiteral("stonecutting")) kind = RecipeKind::Stonecutting;
            else if (shortType == QStringLiteral("smithing_transform")) kind = RecipeKind::SmithingTransform;
            else if (shortType == QStringLiteral("smithing_trim")) kind = RecipeKind::SmithingTrim;
            else if (shortType.startsWith(QStringLiteral("crafting_special_"), Qt::CaseInsensitive)) kind = RecipeKind::Special;

            if (kind == RecipeKind::Unknown) { data.skippedUnknownRecipeCount++; continue; }
            if (kind == RecipeKind::Special || kind == RecipeKind::SmithingTrim) { data.specialRecipeCount++; continue; }

            QVector<IngredientOption> ingredients;
            QString resultItem;
            int resultCount = 1;
            QVector<std::optional<IngredientOption>> gridSlots;
            int gridWidth = 0, gridHeight = 0;
            bool ok = true;

            switch (kind)
            {
            case RecipeKind::CraftingShaped:
            {
                if (!root.contains("pattern") || !root.contains("key")) { ok = false; break; }

                QHash<QChar, IngredientOption> keyMap;
                const QJsonObject keyObj = root.value("key").toObject();
                for (auto it = keyObj.begin(); it != keyObj.end(); ++it)
                {
                    if (it.key().length() != 1) continue;
                    const auto opt = parseIngredientValue(it.value());
                    if (opt) keyMap[it.key().at(0)] = *opt;
                }

                QStringList rows;
                for (const QJsonValue &r : root.value("pattern").toArray())
                    rows.append(r.toString());

                gridHeight = rows.size();
                for (const QString &r : rows) gridWidth = std::max(gridWidth, static_cast<int>(r.length()));
                gridSlots = QVector<std::optional<IngredientOption>>(gridWidth * gridHeight, std::nullopt);

                for (int row = 0; row < gridHeight; ++row)
                {
                    const QString &rowStr = rows[row];
                    for (int col = 0; col < rowStr.length(); ++col)
                    {
                        const QChar ch = rowStr.at(col);
                        if (ch == QLatin1Char(' ')) continue;
                        if (keyMap.contains(ch))
                        {
                            gridSlots[row * gridWidth + col] = keyMap[ch];
                            ingredients.append(keyMap[ch]);
                        }
                    }
                }

                if (!tryParseResult(root, resultItem, resultCount)) ok = false;
                break;
            }

            case RecipeKind::CraftingShapeless:
            {
                if (!root.contains("ingredients")) { ok = false; break; }
                for (const QJsonValue &v : root.value("ingredients").toArray())
                {
                    const auto opt = parseIngredientValue(v);
                    if (opt) ingredients.append(*opt);
                }
                if (!tryParseResult(root, resultItem, resultCount)) ok = false;
                break;
            }

            case RecipeKind::Smelting:
            case RecipeKind::Blasting:
            case RecipeKind::Smoking:
            case RecipeKind::CampfireCooking:
            case RecipeKind::Stonecutting:
            {
                if (!root.contains("ingredient")) { ok = false; break; }
                const auto opt = parseIngredientValue(root.value("ingredient"));
                if (!opt) { ok = false; break; }
                ingredients.append(*opt);
                if (!tryParseResult(root, resultItem, resultCount)) ok = false;
                break;
            }

            case RecipeKind::SmithingTransform:
            {
                if (root.contains("template"))
                {
                    const auto opt = parseIngredientValue(root.value("template"), QStringLiteral("鍛造模板"));
                    if (opt) ingredients.append(*opt);
                }
                if (root.contains("base"))
                {
                    const auto opt = parseIngredientValue(root.value("base"), QStringLiteral("基底材料"));
                    if (opt) ingredients.append(*opt);
                }
                if (root.contains("addition"))
                {
                    const auto opt = parseIngredientValue(root.value("addition"), QStringLiteral("追加材料"));
                    if (opt) ingredients.append(*opt);
                }
                if (!tryParseResult(root, resultItem, resultCount)) ok = false;
                break;
            }

            default:
                ok = false;
                break;
            }

            if (!ok || resultItem.isEmpty()) continue;

            RecipeDefinition def;
            def.id = id;
            def.kind = kind;
            def.resultItem = normalizeId(resultItem);
            def.resultCount = resultCount;
            def.ingredients = ingredients;
            def.group = root.value("group").toString();
            def.gridWidth = gridWidth;
            def.gridHeight = gridHeight;
            def.gridSlots = gridSlots;

            data.recipes.append(def);
            continue;
        }

        // ---- 物品標籤 ----
        if ((m = tagRe.match(name)).hasMatch())
        {
            const QString ns = m.captured(1);
            const QString path = m.captured(2);
            const QString tagId = ns + ":" + path;

            const QJsonDocument doc = readJson();
            if (!doc.isObject()) continue;
            const QJsonObject root = doc.object();
            if (!root.contains("values")) continue;

            QVector<TagEntryRaw> list;
            for (const QJsonValue &v : root.value("values").toArray())
            {
                if (v.isString())
                {
                    QString s = v.toString();
                    const bool isTag = s.startsWith(QLatin1Char('#'));
                    if (isTag) s = s.mid(1);
                    list.append(TagEntryRaw{normalizeId(s), isTag, true});
                }
                else if (v.isObject())
                {
                    const QJsonObject obj = v.toObject();
                    if (!obj.contains("id") || !obj.value("id").isString()) continue;
                    QString s = obj.value("id").toString();
                    const bool required = !obj.contains("required") || obj.value("required").toBool(true);
                    const bool isTag = s.startsWith(QLatin1Char('#'));
                    if (isTag) s = s.mid(1);
                    list.append(TagEntryRaw{normalizeId(s), isTag, required});
                }
            }

            data.itemTags[tagId] = list;
            continue;
        }

        // ---- 語言檔 ----
        if ((m = langRe.match(name)).hasMatch())
        {
            const bool isEnUs = m.captured(1).compare(QStringLiteral("en_us"), Qt::CaseInsensitive) == 0;
            const QJsonDocument doc = readJson();
            if (!doc.isObject()) continue;

            QHash<QString, QString> &target = isEnUs ? data.langEnUs : data.langZhTw;
            const QJsonObject root = doc.object();
            for (auto it = root.begin(); it != root.end(); ++it)
            {
                if (it.value().isString())
                    target[it.key()] = it.value().toString();
            }
            continue;
        }

        // ---- 物品模型（判斷立體方塊外觀用） ----
        if ((m = modelRe.match(name)).hasMatch())
        {
            const QString ns = m.captured(1);
            const QString path = m.captured(2);
            const QString itemId = ns + ":" + path;

            const QJsonDocument doc = readJson();
            if (!doc.isObject()) continue;
            const QJsonObject root = doc.object();

            QString parent;
            if (root.contains("parent") && root.value("parent").isString())
                parent = root.value("parent").toString();

            data.itemModelParents[itemId] = parent;
            continue;
        }
    }

    zip.close();
    return data;
}

} // namespace Services
