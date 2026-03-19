#include "content_store.h"
#include "module_proxy.h"

#include <QFile>
#include <QTemporaryFile>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>

ContentStore::ContentStore(QObject* parent)
    : QObject(parent)
{}

void ContentStore::setStorageClient(ModuleProxy* storage)
{
    m_storage = storage;
}

QString ContentStore::store(const QByteArray& content, int chunkSize)
{
    if (!m_storage) return {};

    // Write content to a temporary file so we can pass a QUrl to uploadUrl().
    QTemporaryFile tmp;
    tmp.setAutoRemove(false);
    if (!tmp.open()) {
        emit error("ContentStore: failed to create temporary upload file");
        return {};
    }
    tmp.write(content);
    tmp.close();

    const QUrl fileUrl = QUrl::fromLocalFile(tmp.fileName());
    // uploadUrl(localFileUrl, chunkSize) → returns CID string.
    const QVariant result = m_storage->invokeRemoteMethod(
        "storage_module", "uploadUrl", fileUrl.toString(), chunkSize);

    QFile::remove(tmp.fileName());

    const QString cid = result.toString().trimmed();
    if (cid.isEmpty()) {
        emit error("ContentStore: storage module returned empty CID");
        return {};
    }

    emit stored(cid);
    return cid;
}

QByteArray ContentStore::fetch(const QString& cid)
{
    if (!m_storage || cid.isEmpty()) return {};

    // Strategy 1: downloadToUrl(cid, destUrl) — writes to a local temp file.
    QTemporaryFile tmp;
    tmp.setAutoRemove(false);
    if (tmp.open()) {
        const QString tmpPath = tmp.fileName();
        tmp.close();

        const QUrl destUrl = QUrl::fromLocalFile(tmpPath);
        m_storage->invokeRemoteMethod(
            "storage_module", "downloadToUrl", cid, destUrl.toString());

        QFile f(tmpPath);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray data = f.readAll();
            f.close();
            QFile::remove(tmpPath);
            if (!data.isEmpty()) {
                emit fetched(cid, data);
                return data;
            }
        }
        QFile::remove(tmpPath);
    }

    // Strategy 2: downloadChunks(cid) → JSON array of base64-encoded chunks.
    const QVariant chunksResult = m_storage->invokeRemoteMethod(
        "storage_module", "downloadChunks", cid);
    const QString chunksJson = chunksResult.toString();
    if (!chunksJson.isEmpty()) {
        const QJsonArray chunks = QJsonDocument::fromJson(chunksJson.toUtf8()).array();
        QByteArray assembled;
        for (const auto& chunk : chunks) {
            assembled += QByteArray::fromBase64(chunk.toString().toLatin1());
        }
        if (!assembled.isEmpty()) {
            emit fetched(cid, assembled);
            return assembled;
        }
    }

    emit error("ContentStore: failed to fetch CID: " + cid);
    return {};
}

bool ContentStore::exists(const QString& cid)
{
    if (!m_storage || cid.isEmpty()) return false;
    const QVariant result = m_storage->invokeRemoteMethod(
        "storage_module", "exists", cid);
    return result.toBool();
}

bool ContentStore::remove(const QString& cid)
{
    if (!m_storage || cid.isEmpty()) return false;
    const QVariant result = m_storage->invokeRemoteMethod(
        "storage_module", "remove", cid);
    return result.toBool();
}
