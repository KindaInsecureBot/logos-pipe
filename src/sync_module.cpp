#include "sync_module.h"
#include "sync_types.h"
#include "logos_api_client.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

SyncModule::SyncModule(QObject* parent)
    : QObject(parent)
    , m_contentStore(new ContentStore(this))
    , m_channelSync(new ChannelSync(this))
    , m_peerSync(new PeerSync(this))
{
    connectSignals();
}

void SyncModule::connectSignals()
{
    connect(m_contentStore, &ContentStore::stored,
            this, &SyncModule::stored);
    connect(m_contentStore, &ContentStore::fetched,
            this, &SyncModule::fetched);
    connect(m_contentStore, &ContentStore::error,
            this, [this](const QString& msg) { emit syncError("content", msg); });

    connect(m_channelSync, &ChannelSync::inscribed,
            this, &SyncModule::inscribed);
    connect(m_channelSync, &ChannelSync::inscriptionReceived,
            this, &SyncModule::inscriptionReceived);
    connect(m_channelSync, &ChannelSync::error,
            this, [this](const QString& msg) { emit syncError("channel", msg); });

    connect(m_peerSync, &PeerSync::messageReceived,
            this, &SyncModule::messageReceived);
    connect(m_peerSync, &PeerSync::started,
            this, &SyncModule::peerSyncStarted);
    connect(m_peerSync, &PeerSync::error,
            this, [this](const QString& msg) { emit syncError("peer", msg); });
}

void SyncModule::initLogos(LogosAPI* api)
{
    if (LogosAPIClient* storage = api->getClient("storage_module"))
        m_contentStore->setStorageClient(storage);

    if (LogosAPIClient* blockchain = api->getClient("blockchain_module")) {
        m_channelSync->setBlockchainClient(blockchain);
        connectBlockchainModule(api);
    }

    if (LogosAPIClient* chat = api->getClient("chat_module")) {
        m_peerSync->setChatClient(chat);
        connectChatModule(api);
    }

    m_peerSync->start();
}

void SyncModule::connectBlockchainModule(LogosAPI* api)
{
    // blockchain_module fires: inscriptionReceived(channelId, inscriptionId, dataHex)
    LogosAPIClient* client = api->getClient("blockchain_module");
    client->onEvent(nullptr, nullptr, "inscriptionReceived",
        [this](const QString& /*eventName*/, const QVariantList& data) {
        if (data.size() < 3) return;
        const QString channelId     = data.value(0).toString();
        const QString inscriptionId = data.value(1).toString();
        const QByteArray dataBytes  = QByteArray::fromHex(
            data.value(2).toString().toLatin1());
        m_channelSync->onInscription(channelId, inscriptionId, dataBytes);
    });
}

void SyncModule::connectChatModule(LogosAPI* api)
{
    // chat_module fires: messageReceived(convoId, senderPubkey, contentHex)
    LogosAPIClient* client = api->getClient("chat_module");
    client->onEvent(nullptr, nullptr, "messageReceived",
        [this](const QString& /*eventName*/, const QVariantList& data) {
        if (data.size() < 3) return;
        const QString convoId      = data.value(0).toString();
        const QString senderPubkey = data.value(1).toString();
        const QString contentHex   = data.value(2).toString();
        m_peerSync->onMessage(convoId, senderPubkey, contentHex);
    });
}

QString SyncModule::store(const QByteArray& content, int chunkSize)
{
    return m_contentStore->store(content, chunkSize);
}

QByteArray SyncModule::fetch(const QString& cid)
{
    return m_contentStore->fetch(cid);
}

bool SyncModule::contentExists(const QString& cid)
{
    return m_contentStore->exists(cid);
}

bool SyncModule::contentRemove(const QString& cid)
{
    return m_contentStore->remove(cid);
}

// static
QString SyncModule::deriveChannelId(const QString& appPrefix, const QString& uniqueId)
{
    return ChannelSync::deriveChannelId(appPrefix, uniqueId);
}

QString SyncModule::inscribe(const QString& channelId, const QByteArray& data)
{
    return m_channelSync->inscribe(channelId, data);
}

QString SyncModule::queryChannel(const QString& channelId)
{
    return m_channelSync->queryChannel(channelId);
}

void SyncModule::follow(const QString& channelId)
{
    m_channelSync->follow(channelId);
}

void SyncModule::unfollow(const QString& channelId)
{
    m_channelSync->unfollow(channelId);
}

void SyncModule::setAppPrefix(const QString& appPrefix)
{
    m_peerSync->setAppPrefix(appPrefix);
}

void SyncModule::setOwnPubkey(const QString& pubkeyHex)
{
    m_peerSync->setOwnPubkey(pubkeyHex);
}

void SyncModule::broadcast(const QByteArray& message)
{
    m_peerSync->broadcast(message);
}

void SyncModule::peerSubscribe(const QString& pubkeyHex)
{
    m_peerSync->subscribe(pubkeyHex);
}

void SyncModule::peerUnsubscribe(const QString& pubkeyHex)
{
    m_peerSync->unsubscribe(pubkeyHex);
}
