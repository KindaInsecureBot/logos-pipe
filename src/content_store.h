#pragma once
#include <QObject>
#include <QByteArray>
#include <QString>

class LogosAPIClient;

// ContentStore — content-addressed storage wrapper (Logos Storage module).
//
// Wraps org.logos.StorageModuleInterface to provide a clean store/fetch API
// for arbitrary blobs identified by CID.  Used by all Basecamp plugins that
// need to persist or retrieve content beyond the kv_module size limit.
//
// Usage:
//   QString cid = contentStore->store(jsonBytes);
//   QByteArray data = contentStore->fetch(cid);
class ContentStore : public QObject {
    Q_OBJECT
public:
    explicit ContentStore(QObject* parent = nullptr);

    void setStorageClient(LogosAPIClient* storage);
    bool isAvailable() const { return m_storage != nullptr; }

    // Upload content bytes to storage.
    // Uses uploadUrl() with a temporary local file, then falls back to
    // per-chunk upload if needed.  Returns the CID string on success,
    // or empty string on failure.
    QString store(const QByteArray& content, int chunkSize = 256 * 1024);

    // Download content by CID.
    // Tries downloadToUrl() first, falls back to downloadChunks().
    // Returns raw bytes, or empty on failure.
    QByteArray fetch(const QString& cid);

    // Check whether a CID is locally available.
    bool exists(const QString& cid);

    // Remove a CID from local storage.
    bool remove(const QString& cid);

signals:
    void stored(const QString& cid);
    void fetched(const QString& cid, const QByteArray& content);
    void error(const QString& message);

private:
    LogosAPIClient* m_storage = nullptr;
};
