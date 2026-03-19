#include "sync_module.h"
#include "sync_types.h"
#include "module_proxy.h"

SyncModule::SyncModule(QObject* parent)
    : QObject(parent)
    , m_contentStore(new LogosSync::ContentStore(this))
    , m_channelSync(new LogosSync::ChannelSync(this))
    , m_peerSync(new LogosSync::PeerSync(this))
{
    connectSignals();
}

void SyncModule::connectSignals()
{
    connect(m_contentStore, &LogosSync::ContentStore::stored,
            this, &SyncModule::stored);
    connect(m_contentStore, &LogosSync::ContentStore::fetched,
            this, &SyncModule::fetched);
    connect(m_contentStore, &LogosSync::ContentStore::error,
            this, [this](const QString& msg) { emit syncError("content", msg); });

    connect(m_channelSync, &LogosSync::ChannelSync::inscribed,
            this, &SyncModule::inscribed);
    connect(m_channelSync, &LogosSync::ChannelSync::inscriptionReceived,
            this, &SyncModule::inscriptionReceived);
    connect(m_channelSync, &LogosSync::ChannelSync::error,
            this, [this](const QString& msg) { emit syncError("channel", msg); });

    connect(m_peerSync, &LogosSync::PeerSync::messageReceived,
            this, &SyncModule::messageReceived);
    connect(m_peerSync, &LogosSync::PeerSync::started,
            this, &SyncModule::peerSyncStarted);
    connect(m_peerSync, &LogosSync::PeerSync::error,
            this, [this](const QString& msg) { emit syncError("peer", msg); });
}

void SyncModule::initLogos(LogosAPI* api)
{
    if (ModuleProxy* storage = api->getClient("storage_module"))
        m_contentStore->setStorageClient(storage);

    if (ModuleProxy* zone = api->getClient("zone_module")) {
        m_channelSync->setZoneClient(zone);
        connectZoneModule(api);
    }

    if (ModuleProxy* chat = api->getClient("chat_module")) {
        m_peerSync->setChatClient(chat);
        connectChatModule(api);
    }

    m_peerSync->start();
}

void SyncModule::connectZoneModule(LogosAPI* api)
{
    // zone_module fires: inscriptionReceived(moduleId, channelId, inscriptionId, dataHex)
    api->on("zone_module", "inscriptionReceived", [this](QVariantList args) {
        if (args.size() < 4) return;
        const QString channelId     = args.value(1).toString();
        const QString inscriptionId = args.value(2).toString();
        const QByteArray data       = QByteArray::fromHex(
            args.value(3).toString().toLatin1());
        m_channelSync->onZoneInscription(channelId, inscriptionId, data);
    });
}

void SyncModule::connectChatModule(LogosAPI* api)
{
    // chat_module fires: messageReceived(moduleId, convoId, senderPubkey, contentHex)
    api->on("chat_module", "messageReceived", [this](QVariantList args) {
        if (args.size() < 4) return;
        const QString convoId      = args.value(1).toString();
        const QString senderPubkey = args.value(2).toString();
        const QString contentHex   = args.value(3).toString();
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
    return LogosSync::ChannelSync::deriveChannelId(appPrefix, uniqueId);
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
