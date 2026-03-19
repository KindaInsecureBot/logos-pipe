#include "channel_sync.h"
#include "sync_types.h"
#include "module_proxy.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

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
QString ChannelSync::deriveChannelId(const QString& appPrefix, const QString& uniqueId)
{
    return LogosSync::deriveChannelId(appPrefix, uniqueId);
}

QString ChannelSync::inscribe(const QString& channelId, const QByteArray& data)
{
    if (!m_blockchain || channelId.isEmpty() || data.isEmpty()) return {};

    // Hex-encode data for wire transport.
    const QString dataHex = QString::fromLatin1(data.toHex());
    const QVariant result = m_blockchain->invokeRemoteMethod(
        "zone_module", "inscribe", channelId, dataHex, m_signingKey);

    const QString inscriptionId = result.toString().trimmed();
    if (inscriptionId.isEmpty()) {
        emit error("inscribe",
                   "zone module returned empty inscription ID for channel: " + channelId);
        return {};
    }

    emit inscribed(channelId, inscriptionId);
    return inscriptionId;
}

QList<QPair<QString, QByteArray>> ChannelSync::queryChannel(const QString& channelId)
{
    if (!m_blockchain || channelId.isEmpty()) return {};

    // zone_module returns a JSON array:
    // [{"id":"<inscriptionId>", "data":"<hex>", "timestamp":"..."}, ...]
    const QVariant result = m_blockchain->invokeRemoteMethod(
        "zone_module", "queryChannel", channelId);
    const QString json = result.toString();
    if (json.isEmpty()) return {};

    const QJsonArray arr = QJsonDocument::fromJson(json.toUtf8()).array();
    QList<QPair<QString, QByteArray>> out;
    out.reserve(arr.size());
    for (const auto& item : arr) {
        const QJsonObject obj = item.toObject();
        const QString id     = obj["id"].toString();
        const QByteArray raw = QByteArray::fromHex(obj["data"].toString().toLatin1());
        if (!id.isEmpty()) out.append({id, raw});
    }
    return out;
}

void ChannelSync::follow(const QString& channelId)
{
    if (channelId.isEmpty() || m_followedChannels.contains(channelId)) return;
    m_followedChannels.insert(channelId);

    if (!m_blockchain) return;
    m_blockchain->invokeRemoteMethod("zone_module", "follow", channelId);
}

void ChannelSync::unfollow(const QString& channelId)
{
    if (channelId.isEmpty()) return;
    m_followedChannels.remove(channelId);

    if (!m_blockchain) return;
    m_blockchain->invokeRemoteMethod("zone_module", "unfollow", channelId);
}

void ChannelSync::onInscription(const QString& channelId,
                                const QString& inscriptionId,
                                const QByteArray& data)
{
    if (!m_followedChannels.contains(channelId)) return;
    if (channelId.isEmpty() || inscriptionId.isEmpty()) return;

    emit inscriptionReceived(channelId, inscriptionId, data);
}
