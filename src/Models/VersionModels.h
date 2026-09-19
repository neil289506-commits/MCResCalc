#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>

namespace Models {

struct VersionEntry
{
    QString id;
    QString type;   // "release" / "snapshot" / "old_beta" / "old_alpha"
    QString url;
    QDateTime releaseTime;

    static VersionEntry fromJson(const QJsonObject &obj)
    {
        VersionEntry v;
        v.id = obj.value("id").toString();
        v.type = obj.value("type").toString();
        v.url = obj.value("url").toString();
        v.releaseTime = QDateTime::fromString(obj.value("releaseTime").toString(), Qt::ISODate);
        return v;
    }
};

struct DownloadArtifact
{
    QString url;
    QString sha1;
    qint64 size = 0;
    bool isValid = false;

    static DownloadArtifact fromJson(const QJsonObject &obj)
    {
        DownloadArtifact d;
        d.url = obj.value("url").toString();
        d.sha1 = obj.value("sha1").toString();
        d.size = static_cast<qint64>(obj.value("size").toDouble());
        d.isValid = !d.url.isEmpty();
        return d;
    }
};

struct VersionDetail
{
    QString id;
    DownloadArtifact client;

    static VersionDetail fromJson(const QJsonObject &obj)
    {
        VersionDetail v;
        v.id = obj.value("id").toString();
        const QJsonObject downloads = obj.value("downloads").toObject();
        v.client = DownloadArtifact::fromJson(downloads.value("client").toObject());
        return v;
    }
};

} // namespace Models
