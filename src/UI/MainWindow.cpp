#include "MainWindow.h"
#include "Block3DPreview.h"
#include "../Services/GameCacheService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QSize>
#include <QtConcurrent>
#include <QSet>
#include <QPair>
#include <algorithm>
#include <optional>

using Models::CraftingSlot;
using Models::CraftingStep;
using Models::MaterialNode;
using Models::RecipeDefinition;
using Models::RecipeKind;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_api(new Services::MojangApiService(this))
    , m_extractWatcher(new QFutureWatcher<Services::ExtractedGameData>(this))
{
    buildUi();

    connect(m_api, &Services::MojangApiService::releaseVersionsReady, this, &MainWindow::onReleaseVersionsReady);
    connect(m_api, &Services::MojangApiService::releaseVersionsError, this, &MainWindow::onReleaseVersionsError);
    connect(m_api, &Services::MojangApiService::versionDetailReady, this, &MainWindow::onVersionDetailReady);
    connect(m_api, &Services::MojangApiService::versionDetailError, this, &MainWindow::onVersionDetailError);
    connect(m_api, &Services::MojangApiService::jarDownloadProgress, this, &MainWindow::onJarDownloadProgress);
    connect(m_api, &Services::MojangApiService::jarReady, this, &MainWindow::onJarReady);
    connect(m_api, &Services::MojangApiService::jarError, this, &MainWindow::onJarError);
    connect(m_extractWatcher, &QFutureWatcher<Services::ExtractedGameData>::finished, this, &MainWindow::onExtractionFinished);

    setStatus(QStringLiteral("尚未載入任何版本。請選擇版本後按下「載入配方資料」。"));
    refreshCalculateEnabled();

    onLoadVersionsClicked();
}

// =====================================================================
// UI 建構
// =====================================================================

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    QFont boldFont = font();
    boldFont.setBold(true);

    // ---- 頂端：版本選擇 ----
    auto *headerCard = new QFrame(central);
    headerCard->setObjectName(QStringLiteral("Card"));
    auto *headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 14, 14, 14);

    auto *titleBox = new QVBoxLayout();
    auto *titleLabel = new QLabel(QStringLiteral("Minecraft 資源計算機"), headerCard);
    QFont titleFont = font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    auto *subtitleLabel = new QLabel(QStringLiteral("輸入想製作的物品與數量，自動展開整棵合成樹並統計最終需要的原始材料"), headerCard);
    subtitleLabel->setObjectName(QStringLiteral("Secondary"));
    titleBox->addWidget(titleLabel);
    titleBox->addWidget(subtitleLabel);
    headerLayout->addLayout(titleBox);
    headerLayout->addStretch();

    m_versionCombo = new QComboBox(headerCard);
    m_versionCombo->setMinimumWidth(200);
    headerLayout->addWidget(m_versionCombo);

    m_refreshVersionsBtn = new QPushButton(QStringLiteral("重新整理版本"), headerCard);
    m_refreshVersionsBtn->setObjectName(QStringLiteral("GhostButton"));
    headerLayout->addWidget(m_refreshVersionsBtn);

    m_loadDataBtn = new QPushButton(QStringLiteral("載入配方資料"), headerCard);
    headerLayout->addWidget(m_loadDataBtn);

    m_englishCheck = new QCheckBox(QStringLiteral("English names"), headerCard);
    headerLayout->addWidget(m_englishCheck);

    outer->addWidget(headerCard);

    // ---- 狀態列 / 進度條 ----
    auto *statusCard = new QFrame(central);
    statusCard->setObjectName(QStringLiteral("Card"));
    auto *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(14, 14, 14, 14);
    m_statusLabel = new QLabel(statusCard);
    m_statusLabel->setWordWrap(true);
    statusLayout->addWidget(m_statusLabel);
    m_progressBar = new QProgressBar(statusCard);
    m_progressBar->setRange(0, 1000);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);
    statusLayout->addWidget(m_progressBar);
    outer->addWidget(statusCard);

    // ---- 主要內容 ----
    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(12);

    // 左側：搜尋 + 待計算清單
    auto *leftCard = new QFrame(central);
    leftCard->setObjectName(QStringLiteral("Card"));
    leftCard->setFixedWidth(380);
    auto *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(14, 14, 14, 14);

    auto *searchLabel = new QLabel(QStringLiteral("搜尋可製作物品"), leftCard);
    searchLabel->setFont(boldFont);
    leftLayout->addWidget(searchLabel);

    m_searchEdit = new QLineEdit(leftCard);
    m_searchEdit->setPlaceholderText(QStringLiteral("輸入開頭字母，例如輸入 o 會列出所有以 o 開頭且有配方的物品"));
    leftLayout->addWidget(m_searchEdit);

    auto *suggestionsLabel = new QLabel(QStringLiteral("搜尋結果（點選以選取）"), leftCard);
    suggestionsLabel->setObjectName(QStringLiteral("Secondary"));
    leftLayout->addWidget(suggestionsLabel);

    m_suggestionsList = new QListWidget(leftCard);
    m_suggestionsList->setFixedHeight(150);
    m_suggestionsList->setIconSize(QSize(20, 20));
    leftLayout->addWidget(m_suggestionsList);

    auto *addRow = new QHBoxLayout();
    m_selectedSuggestionLabel = new QLabel(leftCard);
    m_selectedSuggestionLabel->setObjectName(QStringLiteral("Secondary"));
    addRow->addWidget(m_selectedSuggestionLabel, 1);
    m_quantityToAddSpin = new QSpinBox(leftCard);
    m_quantityToAddSpin->setRange(1, 999999);
    m_quantityToAddSpin->setValue(1);
    m_quantityToAddSpin->setFixedWidth(80);
    addRow->addWidget(m_quantityToAddSpin);
    m_addToBatchBtn = new QPushButton(QStringLiteral("加入清單"), leftCard);
    m_addToBatchBtn->setEnabled(false);
    addRow->addWidget(m_addToBatchBtn);
    leftLayout->addLayout(addRow);

    auto *batchLabel = new QLabel(QStringLiteral("待計算清單"), leftCard);
    batchLabel->setFont(boldFont);
    leftLayout->addWidget(batchLabel);

    m_batchListWidget = new QListWidget(leftCard);
    leftLayout->addWidget(m_batchListWidget, 1);

    contentLayout->addWidget(leftCard);

    // 右側：物品預覽 + 計算結果
    auto *rightCard = new QFrame(central);
    rightCard->setObjectName(QStringLiteral("Card"));
    auto *rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(14, 14, 14, 14);

    auto *calcRow = new QHBoxLayout();
    m_calculateBtn = new QPushButton(QStringLiteral("計算所需原始材料"), rightCard);
    m_calculateBtn->setEnabled(false);
    calcRow->addWidget(m_calculateBtn);
    calcRow->addStretch();
    rightLayout->addLayout(calcRow);

    // 物品預覽：點選左側搜尋結果／原始材料清單／合成樹節點都會更新這裡。
    // 3D 立方體只有「立體方塊外觀」的物品才會顯示，可以用滑鼠拖曳旋轉；
    // 其餘工具/食物這類本來就是平面貼圖的物品維持 2D 圖示，跟遊戲裡的外觀一致。
    auto *previewCard = new QFrame(rightCard);
    previewCard->setObjectName(QStringLiteral("Card"));
    auto *previewLayout = new QHBoxLayout(previewCard);
    previewLayout->setContentsMargins(10, 10, 10, 10);

    m_preview2DIcon = new QLabel(previewCard);
    m_preview2DIcon->setFixedSize(90, 90);
    m_preview2DIcon->setAlignment(Qt::AlignCenter);

    m_preview3D = new UI::Block3DPreview(previewCard);
    m_preview3D->setFixedSize(90, 90);

    m_previewStack = new QStackedWidget(previewCard);
    m_previewStack->setFixedSize(90, 90);
    m_previewStack->addWidget(m_preview2DIcon);
    m_previewStack->addWidget(m_preview3D);
    previewLayout->addWidget(m_previewStack);

    auto *previewTextBox = new QVBoxLayout();
    m_previewNameLabel = new QLabel(previewCard);
    QFont previewNameFont = font();
    previewNameFont.setBold(true);
    previewNameFont.setPointSize(previewNameFont.pointSize() + 1);
    m_previewNameLabel->setFont(previewNameFont);
    m_previewIdLabel = new QLabel(previewCard);
    m_previewIdLabel->setObjectName(QStringLiteral("Secondary"));
    m_previewHintLabel = new QLabel(QStringLiteral("立體方塊模型，可拖曳旋轉查看"), previewCard);
    m_previewHintLabel->setObjectName(QStringLiteral("Secondary"));
    m_previewHintLabel->setVisible(false);
    previewTextBox->addWidget(m_previewNameLabel);
    previewTextBox->addWidget(m_previewIdLabel);
    previewTextBox->addWidget(m_previewHintLabel);
    previewTextBox->addStretch();
    previewLayout->addLayout(previewTextBox, 1);

    rightLayout->addWidget(previewCard);

    m_tabs = new QTabWidget(rightCard);

    m_rawMaterialsListWidget = new QListWidget(m_tabs);
    m_rawMaterialsListWidget->setIconSize(QSize(22, 22));
    m_tabs->addTab(m_rawMaterialsListWidget, QStringLiteral("原始材料統計"));

    m_resultTreeWidget = new QTreeWidget(m_tabs);
    m_resultTreeWidget->setHeaderHidden(true);
    m_resultTreeWidget->setIconSize(QSize(20, 20));
    m_tabs->addTab(m_resultTreeWidget, QStringLiteral("完整合成樹"));

    auto *craftingScroll = new QScrollArea(m_tabs);
    craftingScroll->setWidgetResizable(true);
    m_craftingStepsContainer = new QWidget(craftingScroll);
    m_craftingStepsLayout = new QVBoxLayout(m_craftingStepsContainer);
    m_craftingStepsLayout->addStretch();
    craftingScroll->setWidget(m_craftingStepsContainer);
    m_tabs->addTab(craftingScroll, QStringLiteral("合成教學"));

    m_warningsListWidget = new QListWidget(m_tabs);
    m_tabs->addTab(m_warningsListWidget, QStringLiteral("警告訊息"));

    rightLayout->addWidget(m_tabs, 1);

    contentLayout->addWidget(rightCard, 1);
    outer->addLayout(contentLayout, 1);

    // ---- 事件連接 ----
    connect(m_refreshVersionsBtn, &QPushButton::clicked, this, &MainWindow::onLoadVersionsClicked);
    connect(m_loadDataBtn, &QPushButton::clicked, this, &MainWindow::onLoadRecipeDataClicked);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_suggestionsList, &QListWidget::currentRowChanged, this, [this](int) { onSuggestionSelectionChanged(); });
    connect(m_addToBatchBtn, &QPushButton::clicked, this, &MainWindow::onAddToBatchClicked);
    connect(m_calculateBtn, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
    connect(m_englishCheck, &QCheckBox::toggled, this, &MainWindow::onEnglishToggled);
    connect(m_rawMaterialsListWidget, &QListWidget::currentRowChanged, this, [this](int) { onRawMaterialSelectionChanged(); });
    connect(m_resultTreeWidget, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *, QTreeWidgetItem *) { onTreeSelectionChanged(); });
}

// =====================================================================
// 狀態列 / 忙碌狀態
// =====================================================================

void MainWindow::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}

void MainWindow::setBusy(bool busy)
{
    m_progressBar->setVisible(busy);
    m_loadDataBtn->setEnabled(!busy && m_versionCombo->count() > 0);
    m_refreshVersionsBtn->setEnabled(!busy);
}

void MainWindow::refreshCalculateEnabled()
{
    m_calculateBtn->setEnabled(m_dataLoaded && !m_batchItems.isEmpty());
    m_addToBatchBtn->setEnabled(m_suggestionsList->currentRow() >= 0);
}

// =====================================================================
// 版本清單 / 下載 / 解包（MojangApiService 的 signal 都轉接到這裡）
// =====================================================================

void MainWindow::onLoadVersionsClicked()
{
    setBusy(true);
    setStatus(QStringLiteral("正在向 Mojang 取得版本清單..."));
    m_api->fetchReleaseVersions();
}

void MainWindow::onReleaseVersionsReady(QVector<Models::VersionEntry> versions)
{
    m_versions = std::move(versions);
    m_versionCombo->clear();
    for (const auto &v : m_versions)
        m_versionCombo->addItem(v.id);
    if (!m_versions.isEmpty())
        m_versionCombo->setCurrentIndex(0);

    setBusy(false);
    setStatus(QStringLiteral("已取得 %1 個正式版版本（已自動忽略搶先體驗版/快照版），請選擇版本後載入配方資料。")
                  .arg(m_versions.size()));
}

void MainWindow::onReleaseVersionsError(QString message)
{
    setBusy(false);
    setStatus(QStringLiteral("取得版本清單失敗：%1").arg(message));
}

void MainWindow::onLoadRecipeDataClicked()
{
    const int idx = m_versionCombo->currentIndex();
    if (idx < 0 || idx >= m_versions.size()) return;

    const Models::VersionEntry &entry = m_versions[idx];
    m_selectedVersionId = entry.id;
    m_dataLoaded = false;
    refreshCalculateEnabled();

    setBusy(true);
    m_progressBar->setValue(0);
    setStatus(QStringLiteral("正在取得 %1 的版本中繼資料...").arg(entry.id));

    m_api->fetchVersionDetail(entry.url);
}

void MainWindow::onVersionDetailReady(Models::VersionDetail detail)
{
    m_pendingDetail = detail;
    setStatus(QStringLiteral("正在下載 %1 的 client.jar...").arg(m_selectedVersionId));
    m_api->downloadClientJar(m_selectedVersionId, detail.client);
}

void MainWindow::onVersionDetailError(QString message)
{
    setBusy(false);
    setStatus(QStringLiteral("載入失敗：%1").arg(message));
}

void MainWindow::onJarDownloadProgress(double fraction)
{
    // 下載佔進度條 70%，剩下留給解包/解析。
    m_progressBar->setValue(static_cast<int>(fraction * 700));
}

void MainWindow::onJarReady(QString jarPath)
{
    setStatus(QStringLiteral("正在解包並解析配方 (recipe)、標籤 (tag) 與語言檔..."));
    m_progressBar->setValue(750);
    // 丟到背景執行緒跑，避免大 jar 解析時整個 UI 卡住。
    m_extractWatcher->setFuture(QtConcurrent::run(&Services::JarRecipeExtractor::extract, jarPath));
}

void MainWindow::onJarError(QString message)
{
    setBusy(false);
    setStatus(QStringLiteral("載入失敗：%1").arg(message));
}

void MainWindow::onExtractionFinished()
{
    const Services::ExtractedGameData extracted = m_extractWatcher->result();
    m_progressBar->setValue(900);

    m_recipeIndex = std::make_unique<Services::RecipeIndex>(extracted.recipes);
    m_tagResolver = std::make_unique<Services::TagResolver>(extracted.itemTags);
    m_lang = std::make_unique<Services::LangService>(extracted.langEnUs, extracted.langZhTw);
    m_lang->language = m_englishCheck->isChecked() ? Services::DisplayLanguage::English
                                                    : Services::DisplayLanguage::TraditionalChinese;
    m_textures = std::make_unique<Services::TextureService>(
        Services::GameCacheService::clientJarPath(m_selectedVersionId),
        Services::GameCacheService::textureCacheFolder(m_selectedVersionId));
    m_shapeResolver = std::make_unique<Services::ModelShapeResolver>(extracted.itemModelParents);
    m_calculator = std::make_unique<Services::ResourceCalculatorService>(*m_recipeIndex, *m_tagResolver, *m_lang);

    m_calcOptions = Services::CalculationOptions{};
    m_batchItems.clear();
    m_rawMaterialRows.clear();
    m_craftingSteps.clear();
    m_resultRoots.clear();
    rebuildBatchListWidget();
    m_resultTreeWidget->clear();
    m_rawMaterialsListWidget->clear();
    m_warningsListWidget->clear();
    rebuildCraftingStepsWidget();
    setPreview(QString());

    m_dataLoaded = true;
    m_progressBar->setValue(1000);
    setBusy(false);

    QString status = QStringLiteral("完成！共載入 %1 筆可計算配方").arg(extracted.recipes.size());
    if (extracted.specialRecipeCount > 0)
        status += QStringLiteral("，另有 %1 筆內建特殊配方（染色、複製旗幟/附魔書、鎧甲紋樣等）無法量化已略過")
                      .arg(extracted.specialRecipeCount);
    if (extracted.skippedUnknownRecipeCount > 0)
        status += QStringLiteral("，%1 筆因格式無法辨識而跳過").arg(extracted.skippedUnknownRecipeCount);
    status += QStringLiteral("。");
    setStatus(status);

    updateSuggestions();
    refreshCalculateEnabled();
}

// =====================================================================
// 搜尋 / 待計算清單
// =====================================================================

void MainWindow::updateSuggestions()
{
    m_suggestionsList->clear();
    m_suggestionItemIds.clear();
    if (!m_recipeIndex || !m_lang) return;

    const QStringList matches = m_recipeIndex->searchByPrefix(m_searchEdit->text());
    int count = 0;
    for (const QString &id : matches)
    {
        if (count >= 50) break;
        const QString name = m_lang->displayName(id);
        auto *item = new QListWidgetItem(name + QStringLiteral("  (") + id + QLatin1Char(')'));
        if (m_textures)
        {
            const QPixmap pix = m_textures->icon(id);
            if (!pix.isNull()) item->setIcon(QIcon(pix));
        }
        m_suggestionsList->addItem(item);
        m_suggestionItemIds.append(id);
        ++count;
    }
}

void MainWindow::onSearchTextChanged(const QString &)
{
    updateSuggestions();
}

void MainWindow::onSuggestionSelectionChanged()
{
    const int row = m_suggestionsList->currentRow();
    if (row < 0 || row >= m_suggestionItemIds.size())
    {
        m_selectedSuggestionLabel->setText(QString());
        refreshCalculateEnabled();
        return;
    }

    const QString itemId = m_suggestionItemIds[row];
    m_selectedSuggestionLabel->setText(m_lang ? m_lang->displayName(itemId) : itemId);
    refreshCalculateEnabled();
    setPreview(itemId);
}

void MainWindow::onAddToBatchClicked()
{
    const int row = m_suggestionsList->currentRow();
    if (row < 0 || row >= m_suggestionItemIds.size() || !m_lang) return;

    const QString itemId = m_suggestionItemIds[row];
    const QString displayName = m_lang->displayName(itemId);
    const int qty = m_quantityToAddSpin->value();

    auto it = std::find_if(m_batchItems.begin(), m_batchItems.end(),
                            [&](const BatchItem &b) { return b.itemId == itemId; });
    if (it != m_batchItems.end())
        it->quantity += qty;
    else
        m_batchItems.append(BatchItem{itemId, displayName, qty});

    rebuildBatchListWidget();
    refreshCalculateEnabled();
}

void MainWindow::rebuildBatchListWidget()
{
    m_batchListWidget->clear();

    for (int i = 0; i < m_batchItems.size(); ++i)
    {
        auto *item = new QListWidgetItem(m_batchListWidget);
        auto *row = new QWidget();
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 2, 4, 2);

        auto *nameLabel = new QLabel(m_batchItems[i].displayName, row);
        rowLayout->addWidget(nameLabel, 1);

        auto *qtySpin = new QSpinBox(row);
        qtySpin->setRange(1, 999999);
        qtySpin->setValue(m_batchItems[i].quantity);
        qtySpin->setFixedWidth(80);
        connect(qtySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, i](int value) {
            if (i < m_batchItems.size())
                m_batchItems[i].quantity = value;
        });
        rowLayout->addWidget(qtySpin);

        auto *removeBtn = new QPushButton(QStringLiteral("移除"), row);
        removeBtn->setObjectName(QStringLiteral("DangerButton"));
        connect(removeBtn, &QPushButton::clicked, this, [this, i]() {
            if (i < m_batchItems.size())
            {
                m_batchItems.removeAt(i);
                rebuildBatchListWidget();
                refreshCalculateEnabled();
            }
        });
        rowLayout->addWidget(removeBtn);

        item->setSizeHint(row->sizeHint());
        m_batchListWidget->setItemWidget(item, row);
    }
}

// =====================================================================
// 物品預覽（2D / 3D 切換）
// =====================================================================

void MainWindow::setPreview(const QString &itemId)
{
    if (itemId.isEmpty() || !m_lang)
    {
        m_previewNameLabel->setText(QString());
        m_previewIdLabel->setText(QString());
        m_preview2DIcon->clear();
        m_previewHintLabel->setVisible(false);
        m_previewStack->setCurrentWidget(m_preview2DIcon);
        return;
    }

    const QString name = m_lang->displayName(itemId);
    m_previewNameLabel->setText(name);
    m_previewIdLabel->setText(itemId);

    const bool isBlock = m_shapeResolver ? m_shapeResolver->isBlockShaped(itemId) : false;
    m_previewHintLabel->setVisible(isBlock);

    if (isBlock && m_textures)
    {
        const auto paths = m_textures->blockFaceTexturePaths(itemId);
        m_preview3D->setTextures(paths.top, paths.side, paths.bottom);
        m_previewStack->setCurrentWidget(m_preview3D);
    }
    else
    {
        const QPixmap pix = m_textures ? m_textures->icon(itemId) : QPixmap();
        if (!pix.isNull())
            m_preview2DIcon->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        else
            m_preview2DIcon->clear();
        m_previewStack->setCurrentWidget(m_preview2DIcon);
    }
}

void MainWindow::onRawMaterialSelectionChanged()
{
    const int row = m_rawMaterialsListWidget->currentRow();
    if (row < 0 || row >= m_rawMaterialRows.size()) return;
    setPreview(m_rawMaterialRows[row].itemId);
}

void MainWindow::onTreeSelectionChanged()
{
    auto *item = m_resultTreeWidget->currentItem();
    if (!item) return;
    const QString itemId = item->data(0, Qt::UserRole).toString();
    if (!itemId.isEmpty()) setPreview(itemId);
}

void MainWindow::onEnglishToggled(bool checked)
{
    if (!m_lang) return;
    m_lang->language = checked ? Services::DisplayLanguage::English : Services::DisplayLanguage::TraditionalChinese;
    updateSuggestions();
    if (m_dataLoaded && !m_batchItems.isEmpty())
        onCalculateClicked();
}

// =====================================================================
// 計算
// =====================================================================

void MainWindow::applyIcons(const std::shared_ptr<MaterialNode> &node)
{
    if (m_textures) node->icon = m_textures->icon(node->itemId);
    for (auto &child : node->children)
        applyIcons(child);
}

void MainWindow::onCalculateClicked()
{
    if (!m_calculator || !m_lang) return;

    QVector<QPair<QString, int>> requests;
    for (const auto &b : m_batchItems)
        requests.append(qMakePair(b.itemId, b.quantity));

    const Services::CalculationResult result = m_calculator->calculate(requests, m_calcOptions);

    m_resultRoots = result.roots;
    for (const auto &root : m_resultRoots)
        applyIcons(root);
    rebuildTreeWidget();

    QVector<QPair<QString, int>> sortedTotals;
    for (auto it = result.rawMaterialTotals.constBegin(); it != result.rawMaterialTotals.constEnd(); ++it)
        sortedTotals.append(qMakePair(it.key(), it.value()));
    std::sort(sortedTotals.begin(), sortedTotals.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) { return a.second > b.second; });

    m_rawMaterialRows.clear();
    for (const auto &kv : sortedTotals)
        m_rawMaterialRows.append(RawMaterialRow{kv.first, m_lang->displayName(kv.first), kv.second});
    rebuildRawMaterialsWidget();

    m_craftingSteps.clear();
    QSet<QString> seen;
    for (const auto &root : m_resultRoots)
        collectCraftingSteps(root, seen);
    rebuildCraftingStepsWidget();

    m_warningsListWidget->clear();
    QSet<QString> seenWarnings;
    for (const QString &w : result.warnings)
    {
        if (seenWarnings.contains(w)) continue;
        seenWarnings.insert(w);
        m_warningsListWidget->addItem(w);
    }
}

QString MainWindow::stacksText(int quantity) const
{
    if (quantity < 64) return QString();
    const int stacks = quantity / 64;
    const int remainder = quantity % 64;
    const int shulkers = stacks / 27;
    const int stacksRemainder = stacks % 27;

    QStringList parts;
    if (shulkers > 0) parts.append(QStringLiteral("%1 個潛影箱").arg(shulkers));
    if (stacksRemainder > 0) parts.append(QStringLiteral("%1 組").arg(stacksRemainder));
    if (remainder > 0) parts.append(QStringLiteral("%1 個").arg(remainder));
    return QStringLiteral("＝ ") + parts.join(QStringLiteral(" + "));
}

void MainWindow::rebuildRawMaterialsWidget()
{
    m_rawMaterialsListWidget->clear();
    for (const auto &row : m_rawMaterialRows)
    {
        auto *item = new QListWidgetItem();
        QString text = QStringLiteral("%1    ×%2").arg(row.displayName).arg(row.quantity);
        const QString stacks = stacksText(row.quantity);
        if (!stacks.isEmpty()) text += QStringLiteral("   %1").arg(stacks);
        item->setText(text);
        item->setData(Qt::UserRole, row.itemId);
        if (m_textures)
        {
            const QPixmap pix = m_textures->icon(row.itemId);
            if (!pix.isNull()) item->setIcon(QIcon(pix));
        }
        m_rawMaterialsListWidget->addItem(item);
    }
}

QTreeWidgetItem *MainWindow::buildTreeItem(const std::shared_ptr<MaterialNode> &node)
{
    auto *item = new QTreeWidgetItem();
    item->setText(0, node->summaryText());
    item->setData(0, Qt::UserRole, node->itemId);
    if (!node->icon.isNull())
        item->setIcon(0, QIcon(node->icon));

    for (const auto &child : node->children)
        item->addChild(buildTreeItem(child));

    return item;
}

void MainWindow::rebuildTreeWidget()
{
    m_resultTreeWidget->clear();
    for (const auto &root : m_resultRoots)
        m_resultTreeWidget->addTopLevelItem(buildTreeItem(root));
    m_resultTreeWidget->expandAll();
}

// ---- 合成教學：依「材料的材料先教」順序（後序走訪、依物品去重）收集步驟 ----

void MainWindow::collectCraftingSteps(const std::shared_ptr<MaterialNode> &node, QSet<QString> &seen)
{
    for (const auto &child : node->children)
        collectCraftingSteps(child, seen);

    if (node->isBase) return;
    if (seen.contains(node->itemId)) return;
    seen.insert(node->itemId);
    if (!node->usedRecipe) return;

    const RecipeDefinition &recipe = *node->usedRecipe;

    CraftingStep step;
    step.resultItemId = node->itemId;
    step.resultDisplayName = node->displayName;
    step.resultIcon = node->icon;
    step.resultCount = recipe.resultCount;
    step.station = recipe.station();
    step.isGrid = (recipe.kind == RecipeKind::CraftingShaped || recipe.kind == RecipeKind::CraftingShapeless);

    if (recipe.kind == RecipeKind::Smelting || recipe.kind == RecipeKind::Blasting ||
        recipe.kind == RecipeKind::Smoking || recipe.kind == RecipeKind::CampfireCooking)
    {
        step.extraNote = QStringLiteral("另外需要放入燃料（煤炭、木炭、烈焰棒等皆可）");
    }

    if (step.isGrid)
    {
        // 注意：這個變數千萬不要命名為 slots —— slots 是 Qt 在 qobjectdefs.h 裡定義的
        // 關鍵字巨集（#define slots，展開成空的），一旦拿來當變數名，
        // slots[i] = ... 在預處理後會變成 [i] = ...，MSVC 會把開頭的 '[' 當成
        // C++ attribute 語法去解析，丟出莫名其妙的 C2337「找不到屬性」。
        QVector<std::optional<CraftingSlot>> gridBuffer(9, std::nullopt);

        if (recipe.kind == RecipeKind::CraftingShaped && !recipe.gridSlots.isEmpty() && recipe.gridWidth > 0)
        {
            int childIndex = 0;
            for (int r = 0; r < std::min(recipe.gridHeight, 3); ++r)
            {
                for (int c = 0; c < std::min(recipe.gridWidth, 3); ++c)
                {
                    const int gridIdx = r * recipe.gridWidth + c;
                    if (gridIdx >= recipe.gridSlots.size() || !recipe.gridSlots[gridIdx]) continue;
                    if (childIndex >= node->children.size()) continue;

                    const auto &child = node->children[childIndex++];
                    gridBuffer[r * 3 + c] = CraftingSlot{child->itemId, child->displayName, child->icon, QString()};
                }
            }
        }
        else
        {
            // 無形狀合成：遊戲裡放哪一格都無所謂，這裡就依序填進 3x3 給玩家一個具體參考。
            // std::min 的兩個引數型別必須一致：children.size() 在 Qt6 回傳 qsizetype
            // （64 位元上是 long long），直接跟字面值 9 (int) 比會推導失敗，所以明確指定型別。
            const qsizetype fillCount = std::min<qsizetype>(9, node->children.size());
            for (qsizetype i = 0; i < fillCount; ++i)
            {
                const auto &child = node->children[i];
                gridBuffer[i] = CraftingSlot{child->itemId, child->displayName, child->icon, QString()};
            }
        }

        step.gridSlots = gridBuffer;
    }
    else
    {
        for (qsizetype i = 0; i < recipe.ingredients.size() && i < node->children.size(); ++i)
        {
            const auto &child = node->children[i];
            const QString label = recipe.ingredients[i].role.isEmpty() ? QStringLiteral("材料") : recipe.ingredients[i].role;
            step.linearSlots.append(CraftingSlot{child->itemId, child->displayName, child->icon, label});
        }
    }

    m_craftingSteps.append(step);
}

QWidget *MainWindow::buildIconCell(const QPixmap &icon, const QString &tooltip, int size, const QString &label)
{
    auto *cell = new QWidget();
    auto *cellLayout = new QVBoxLayout(cell);
    cellLayout->setContentsMargins(0, 0, 0, 0);
    cellLayout->setSpacing(2);

    auto *box = new QFrame(cell);
    box->setFixedSize(size, size);
    box->setStyleSheet(QStringLiteral("background-color:#1a1a1a; border:1px solid #2a2a2a; border-radius:4px;"));
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(4, 4, 4, 4);

    auto *iconLabel = new QLabel(box);
    if (!icon.isNull())
        iconLabel->setPixmap(icon.scaled(size - 8, size - 8, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    if (!tooltip.isEmpty()) box->setToolTip(tooltip);
    boxLayout->addWidget(iconLabel);
    cellLayout->addWidget(box);

    if (!label.isEmpty())
    {
        auto *labelWidget = new QLabel(label, cell);
        labelWidget->setObjectName(QStringLiteral("Secondary"));
        QFont lf = labelWidget->font();
        lf.setPointSize(std::max(8, lf.pointSize() - 1));
        labelWidget->setFont(lf);
        labelWidget->setAlignment(Qt::AlignCenter);
        cellLayout->addWidget(labelWidget);
    }

    return cell;
}

QWidget *MainWindow::buildCraftingStepCard(const CraftingStep &step)
{
    auto *card = new QFrame();
    card->setObjectName(QStringLiteral("Card"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);

    auto *headerRow = new QHBoxLayout();
    auto *iconLabel = new QLabel(card);
    if (!step.resultIcon.isNull())
        iconLabel->setPixmap(step.resultIcon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setFixedSize(24, 24);
    headerRow->addWidget(iconLabel);

    auto *nameLabel = new QLabel(QStringLiteral("%1 × %2").arg(step.resultDisplayName).arg(step.resultCount), card);
    QFont f = font();
    f.setBold(true);
    nameLabel->setFont(f);
    headerRow->addWidget(nameLabel);

    auto *stationLabel = new QLabel(step.station, card);
    stationLabel->setObjectName(QStringLiteral("Secondary"));
    headerRow->addWidget(stationLabel);
    headerRow->addStretch();
    layout->addLayout(headerRow);

    if (step.isGrid)
    {
        auto *grid = new QGridLayout();
        grid->setSpacing(2);
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                const int idx = r * 3 + c;
                const auto slot = step.gridSlots.value(idx);
                const QPixmap icon = slot ? slot->icon : QPixmap();
                const QString tooltip = slot ? slot->displayName : QString();
                grid->addWidget(buildIconCell(icon, tooltip, 44), r, c);
            }
        }
        auto *gridWrap = new QWidget(card);
        gridWrap->setLayout(grid);
        layout->addWidget(gridWrap);
    }
    else
    {
        auto *row = new QHBoxLayout();
        for (const auto &slot : step.linearSlots)
            row->addWidget(buildIconCell(slot.icon, slot.displayName, 44, slot.label));
        row->addStretch();
        layout->addLayout(row);
    }

    if (!step.extraNote.isEmpty())
    {
        auto *noteLabel = new QLabel(step.extraNote, card);
        noteLabel->setObjectName(QStringLiteral("Gold"));
        noteLabel->setWordWrap(true);
        layout->addWidget(noteLabel);
    }

    return card;
}

void MainWindow::rebuildCraftingStepsWidget()
{
    while (m_craftingStepsLayout->count() > 0)
    {
        QLayoutItem *layoutItem = m_craftingStepsLayout->takeAt(0);
        if (layoutItem->widget()) layoutItem->widget()->deleteLater();
        delete layoutItem;
    }

    for (const auto &step : m_craftingSteps)
        m_craftingStepsLayout->addWidget(buildCraftingStepCard(step));

    m_craftingStepsLayout->addStretch();
}
