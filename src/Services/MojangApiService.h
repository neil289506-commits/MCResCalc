#pragma once

#include <QObject>
#include <QVector>
#include "../Models/VersionModels.h"

class QNetworkAccessManager;

namespace Services {

/// <summary>
/// 全部用 QNetworkAccessManager 非同步跑，結果透過 signal 回報。
/// 呼叫端（MainWindow）connect 這些 signal 更新 UI，不需要自己管執行緒。
/// </summary>
class MojangApiService : public QObject
{
    Q_OBJECT
public:
    explicit MojangApiService(QObject *parent = nullptr);

    /// <summary>抓版本清單，只會在 releaseVersionsReady 裡回傳 type == "release" 的項目
    /// （依需求規格，snapshot / old_beta / old_alpha 一律忽略，不會出現在結果裡），
    /// 依發布時間新到舊排序。網路失敗但本機有快取時會退回快取，離線也能用。</summary>
    void fetchReleaseVersions();

    void fetchVersionDetail(const QString &versionMetaUrl);

    /// <summary>下載 client.jar，用 Mojang 公布的 SHA-1 驗證完整性。
    /// 已有驗證通過的本機快取時直接沿用，不重新下載。</summary>
    void downloadClientJar(const QString &versionId, const Models::DownloadArtifact &client);

signals:
    void releaseVersionsReady(QVector<Models::VersionEntry> versions);
    void releaseVersionsError(QString message);

    void versionDetailReady(Models::VersionDetail detail);
    void versionDetailError(QString message);

    void jarDownloadProgress(double fraction);
    void jarReady(QString jarPath);
    void jarError(QString message);

private:
    QNetworkAccessManager *m_net;

    static bool verifySha1(const QString &path, const QString &expectedSha1);
};

} // namespace Services
