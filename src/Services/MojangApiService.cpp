#include "MojangApiService.h"
#include "GameCacheService.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCryptographicHash>
#include <memory>
#include <algorithm>

namespace Services {

namespace {
constexpr const char *kManifestUrl = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json";
}

MojangApiService::MojangApiService(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
}

void MojangApiService::fetchReleaseVersions()
{
    QNetworkRequest req{QUrl(QString::fromLatin1(kManifestUrl))};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("MinecraftResourceCalculator/1.0"));

    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        QByteArray data;
        if (reply->error() == QNetworkReply::NoError)
        {
            data = reply->readAll();
            QFile cacheFile(GameCacheService::manifestCachePath());
            if (cacheFile.open(QIODevice::WriteOnly))
                cacheFile.write(data);
        }
        else
        {
            // 網路失敗，退回本機快取（如果有的話），離線也能用。
            QFile cacheFile(GameCacheService::manifestCachePath());
            if (cacheFile.exists() && cacheFile.open(QIODevice::ReadOnly))
                data = cacheFile.readAll();
        }

        if (data.isEmpty())
        {
            emit releaseVersionsError(QStringLiteral("無法取得或解析版本清單，且沒有可用的本機快取。"));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonArray versionsArr = doc.object().value("versions").toArray();

        QVector<Models::VersionEntry> releases;
        for (const QJsonValue &v : versionsArr)
        {
            Models::VersionEntry entry = Models::VersionEntry::fromJson(v.toObject());
            if (entry.type.compare(QStringLiteral("release"), Qt::CaseInsensitive) == 0)
                releases.push_back(entry);
        }

        std::sort(releases.begin(), releases.end(), [](const Models::VersionEntry &a, const Models::VersionEntry &b) {
            return a.releaseTime > b.releaseTime;
        });

        emit releaseVersionsReady(releases);
    });
}

void MojangApiService::fetchVersionDetail(const QString &versionMetaUrl)
{
    QNetworkReply *reply = m_net->get(QNetworkRequest{QUrl(versionMetaUrl)});
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            emit versionDetailError(reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const Models::VersionDetail detail = Models::VersionDetail::fromJson(doc.object());
        if (!detail.client.isValid)
        {
            emit versionDetailError(QStringLiteral("此版本的中繼資料沒有 client.jar 下載資訊。"));
            return;
        }
        emit versionDetailReady(detail);
    });
}

void MojangApiService::downloadClientJar(const QString &versionId, const Models::DownloadArtifact &client)
{
    const QString path = GameCacheService::clientJarPath(versionId);

    if (QFile::exists(path) && verifySha1(path, client.sha1))
    {
        emit jarDownloadProgress(1.0);
        emit jarReady(path);
        return;
    }

    const QString tempPath = path + ".download";
    auto file = std::make_shared<QFile>(tempPath);
    if (!file->open(QIODevice::WriteOnly))
    {
        emit jarError(QStringLiteral("無法建立暫存檔案：%1").arg(tempPath));
        return;
    }

    QNetworkReply *reply = m_net->get(QNetworkRequest{QUrl(client.url)});

    // 邊收邊寫，避免整包 ~20MB 的 jar 先塞在記憶體裡才寫檔。
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            emit jarDownloadProgress(static_cast<double>(received) / static_cast<double>(total));
    });

    const QString sha1 = client.sha1;
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, path, tempPath, sha1]() {
        reply->deleteLater();
        file->close();

        if (reply->error() != QNetworkReply::NoError)
        {
            QFile::remove(tempPath);
            emit jarError(reply->errorString());
            return;
        }

        if (!verifySha1(tempPath, sha1))
        {
            QFile::remove(tempPath);
            emit jarError(QStringLiteral("下載的 client.jar SHA-1 校驗失敗，檔案可能已損毀，請重試一次。"));
            return;
        }

        QFile::remove(path);
        QFile::rename(tempPath, path);
        emit jarDownloadProgress(1.0);
        emit jarReady(path);
    });
}

bool MojangApiService::verifySha1(const QString &path, const QString &expectedSha1)
{
    if (expectedSha1.isEmpty()) return true;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(&f)) return false;

    return QString::fromLatin1(hash.result().toHex()).compare(expectedSha1, Qt::CaseInsensitive) == 0;
}

} // namespace Services
