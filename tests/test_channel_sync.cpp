#include <QtTest>
#include <QCryptographicHash>
#include "channel_sync.h"
#include "sync_types.h"
#include "logos_api_client.h"

class TestChannelSync : public QObject {
    Q_OBJECT
private slots:
    void deriveChannelId_matchesSyncTypes()
    {
        const QString a = ChannelSync::deriveChannelId("BLOG", "post-1");
        const QString b = LogosSync::deriveChannelId("BLOG", "post-1");
        QCOMPARE(a, b);
    }

    void deriveChannelId_is64HexChars()
    {
        const QString id = ChannelSync::deriveChannelId("NOTES", "note-42");
        QCOMPARE(id.length(), 64);
        QVERIFY(QRegularExpression("^[0-9a-f]{64}$").match(id).hasMatch());
    }

    void deriveChannelId_deterministicAcrossCalls()
    {
        const QString a = ChannelSync::deriveChannelId("WIKI", "index");
        const QString b = ChannelSync::deriveChannelId("WIKI", "index");
        QCOMPARE(a, b);
    }

    void isAvailable_noClient_returnsFalse()
    {
        ChannelSync cs;
        QVERIFY(!cs.isAvailable());
    }

    void isAvailable_withClient_returnsTrue()
    {
        ChannelSync cs;
        LogosAPIClient proxy;
        cs.setBlockchainClient(&proxy);
        QVERIFY(cs.isAvailable());
    }

    void inscribe_noClient_returnsEmpty()
    {
        ChannelSync cs;
        QVERIFY(cs.inscribe("channelid", QByteArray("data")).isEmpty());
    }

    void inscribe_emptyChannelId_returnsEmpty()
    {
        ChannelSync cs;
        LogosAPIClient proxy;
        cs.setBlockchainClient(&proxy);
        QVERIFY(cs.inscribe(QString(), QByteArray("data")).isEmpty());
    }

    void queryChannel_noClient_returnsEmpty()
    {
        ChannelSync cs;
        QVERIFY(cs.queryChannel("channelid").isEmpty());
    }

    void follow_addsToWatchedSet()
    {
        ChannelSync cs;
        const QString channelId = ChannelSync::deriveChannelId("BLOG", "post-1");
        // follow without client — should not crash
        cs.follow(channelId);
        // onInscription on a followed channel emits inscriptionReceived
        QSignalSpy spy(&cs, &ChannelSync::inscriptionReceived);
        cs.onInscription(channelId, "insc-1", QByteArray("payload"));
        QCOMPARE(spy.count(), 1);
    }

    void unfollow_stopsReceivingEvents()
    {
        ChannelSync cs;
        const QString channelId = ChannelSync::deriveChannelId("BLOG", "post-2");
        cs.follow(channelId);
        cs.unfollow(channelId);

        QSignalSpy spy(&cs, &ChannelSync::inscriptionReceived);
        cs.onInscription(channelId, "insc-2", QByteArray("payload"));
        QCOMPARE(spy.count(), 0);
    }

    void onInscription_unknownChannel_notEmitted()
    {
        ChannelSync cs;
        QSignalSpy spy(&cs, &ChannelSync::inscriptionReceived);
        cs.onInscription("unknown-channel", "insc-x", QByteArray("data"));
        QCOMPARE(spy.count(), 0);
    }

    void follow_emptyChannelId_noOp()
    {
        ChannelSync cs;
        // Should not crash.
        cs.follow(QString());
        cs.unfollow(QString());
    }

    void setSigningKey_doesNotCrash()
    {
        ChannelSync cs;
        cs.setSigningKey("deadbeef");
    }
};

QTEST_MAIN(TestChannelSync)
#include "test_channel_sync.moc"
