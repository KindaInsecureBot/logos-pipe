#include "channel_sync.h"
#include "sync_types.h"
#include "module_proxy.h"

ChannelSync::ChannelSync(QObject* parent)
    : QObject(parent)
{}

void ChannelSync::setBlockchainClient(ModuleProxy* blockchain)
{
    m_blockchain = blockchain;
}

void ChannelSync::setSigningKey(const QString& privkeyHex)
{
    m_signingKey = privkeyHex;
}

// static
QString ChannelSync::deriveChannelId(const QString& appPrefix,
                                     const QString& uniqueId)
{
    return LogosSync::deriveChannelId(appPrefix, uniqueId);
}

QString ChannelSync::inscribe(const QString& channelId, const QByteArray& data)
{
    if (!m_blockchain) {
        emit error("ChannelSync: no blockchain client available");
        return {};
    }
    if (channelId.isEmpty() || data.isEmpty()) {
        emit error("ChannelSync: channelId and data must not be empty");
        return {};
    }

    const QString dataHex = QString::fromLatin1(data.toHex());
    const QVariant result = m_blockchain->invokeRemoteMethod(
        "blockchain_module", "inscribe", channelId, dataHex, m_signingKey);

    const QString inscriptionId = result.toString().trimmed();
    if (inscriptionId.isEmpty()) {
        emit error("ChannelSync: blockchain module returned empty inscription ID "
                   "for channel: " + channelId);
        return {};
    }

    emit inscribed(channelId, inscriptionId);
    return inscriptionId;
}

QString ChannelSync::queryChannel(const QString& channelId)
{
    if (!m_blockchain || channelId.isEmpty()) return {};

    const QVariant result = m_blockchain->invokeRemoteMethod(
        "blockchain_module", "queryChannel", channelId);
    const QString json = result.toString();
    return json.isEmpty() ? QStringLiteral("[]") : json;
}

void ChannelSync::follow(const QString& channelId)
{
    if (channelId.isEmpty() || m_followedChannels.contains(channelId)) return;
    m_followedChannels.insert(channelId);

    if (!m_blockchain) return;
    m_blockchain->invokeRemoteMethod("blockchain_module", "followChannel",
                                     channelId);
}

void ChannelSync::unfollow(const QString& channelId)
{
    if (channelId.isEmpty()) return;
    m_followedChannels.remove(channelId);

    if (!m_blockchain) return;
    m_blockchain->invokeRemoteMethod("blockchain_module", "unfollowChannel",
                                     channelId);
}

void ChannelSync::onInscription(const QString& channelId,
                                const QString& inscriptionId,
                                const QByteArray& data)
{
    if (!m_followedChannels.contains(channelId)) return;
    if (channelId.isEmpty() || inscriptionId.isEmpty()) return;

    emit inscriptionReceived(channelId, inscriptionId, data);
}
