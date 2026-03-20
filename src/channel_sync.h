#pragma once
#include <QObject>
#include <QSet>
#include <QString>
#include <QByteArray>

class LogosAPIClient;

// ChannelSync — Zone SDK L1 channel abstraction.
//
// Channels are identified by a deterministic SHA-256 ID derived from an app
// prefix and a unique payload (see sync_types.h::deriveChannelId).
//
//   channelId = SHA-256("λAPP:uniqueId")
//
// Each inscription is a signed data blob anchored on-chain via the blockchain
// module.  Plugins call inscribe() to publish and queryChannel() to retrieve
// full history; follow()/unfollow() control live event delivery.
//
// SyncModule wires the blockchain module's inscriptionReceived signal to
// onInscription().
class ChannelSync : public QObject {
    Q_OBJECT
public:
    explicit ChannelSync(QObject* parent = nullptr);

    void setBlockchainClient(LogosAPIClient* blockchain);
    void setSigningKey(const QString& privkeyHex);
    bool isAvailable() const { return m_blockchain != nullptr; }

    // Derive a deterministic channel ID: SHA-256("λAPP:uniqueId").
    // Mirrors LogosSync::deriveChannelId() from sync_types.h; provided here
    // as a static member for callers that only have a ChannelSync reference.
    static QString deriveChannelId(const QString& appPrefix,
                                   const QString& uniqueId);

    // Inscribe data on the channel.  Data is hex-encoded for wire transport.
    // Returns the inscription ID on success, or empty string on failure.
    QString inscribe(const QString& channelId, const QByteArray& data);

    // Retrieve all inscriptions for a channel.
    // Returns a JSON array string: [{"id":"...", "data":"<hex>"}, ...]
    QString queryChannel(const QString& channelId);

    // Start receiving live inscription events for a channel.
    void follow(const QString& channelId);

    // Stop receiving live events for a channel.
    void unfollow(const QString& channelId);

    // Called by SyncModule when the blockchain module fires inscriptionReceived.
    void onInscription(const QString& channelId,
                       const QString& inscriptionId,
                       const QByteArray& data);

signals:
    void inscribed(const QString& channelId, const QString& inscriptionId);
    void inscriptionReceived(const QString& channelId,
                             const QString& inscriptionId,
                             const QByteArray& data);
    void error(const QString& message);

private:
    LogosAPIClient*  m_blockchain = nullptr;
    QString       m_signingKey;
    QSet<QString> m_followedChannels;
};
