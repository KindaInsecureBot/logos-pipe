#include <QtTest>
#include <QCryptographicHash>
#include "peer_sync.h"
#include "sync_types.h"
#include "module_proxy.h"

class TestPeerSync : public QObject {
    Q_OBJECT
private slots:
    void convoIdForPubkey_matchesDeriveConvoId()
    {
        PeerSync ps;
        ps.setAppPrefix("BLOG");
        const QString pubkey = "aabbccdd";
        QCOMPARE(ps.convoIdForPubkey(pubkey),
                 LogosSync::deriveConvoId("BLOG", pubkey));
    }

    void convoIdForPubkey_defaultPrefix_usesLogos()
    {
        PeerSync ps;
        const QString pubkey = "aabbccdd";
        QCOMPARE(ps.convoIdForPubkey(pubkey),
                 LogosSync::deriveConvoId("logos", pubkey));
    }

    void convoIdForPubkey_changesWithPrefix()
    {
        PeerSync ps;
        ps.setAppPrefix("NOTES");
        const QString pubkey = "aabbccdd";
        QVERIFY(ps.convoIdForPubkey(pubkey) !=
                LogosSync::deriveConvoId("BLOG", pubkey));
    }

    void isAvailable_noClient_returnsFalse()
    {
        PeerSync ps;
        QVERIFY(!ps.isAvailable());
    }

    void isAvailable_withClient_returnsTrue()
    {
        PeerSync ps;
        ModuleProxy proxy;
        ps.setChatClient(&proxy);
        QVERIFY(ps.isAvailable());
    }

    void start_noClient_emitsStarted()
    {
        PeerSync ps;
        QSignalSpy spy(&ps, &PeerSync::started);
        ps.start();
        QCOMPARE(spy.count(), 1);
    }

    void subscribe_thenReceive_emitsMessage()
    {
        PeerSync ps;
        ps.setAppPrefix("BLOG");
        const QString pubkey  = "cafebabe";
        const QString convoId = ps.convoIdForPubkey(pubkey);

        ps.subscribe(pubkey);

        QSignalSpy spy(&ps, &PeerSync::messageReceived);
        const QByteArray payload = QByteArray("hello");
        const QString hex = QString::fromLatin1(payload.toHex());
        ps.onMessage(convoId, pubkey, hex);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), pubkey);
        QCOMPARE(spy.at(0).at(1).toByteArray(), payload);
    }

    void unsubscribe_stopsMessages()
    {
        PeerSync ps;
        ps.setAppPrefix("BLOG");
        const QString pubkey  = "deadbeef";
        const QString convoId = ps.convoIdForPubkey(pubkey);

        ps.subscribe(pubkey);
        ps.unsubscribe(pubkey);

        QSignalSpy spy(&ps, &PeerSync::messageReceived);
        ps.onMessage(convoId, pubkey,
                     QString::fromLatin1(QByteArray("ignored").toHex()));
        QCOMPARE(spy.count(), 0);
    }

    void onMessage_unknownConvo_notEmitted()
    {
        PeerSync ps;
        QSignalSpy spy(&ps, &PeerSync::messageReceived);
        ps.onMessage("unknown-convo", "pubkey",
                     QString::fromLatin1(QByteArray("data").toHex()));
        QCOMPARE(spy.count(), 0);
    }

    void onMessage_invalidHex_notEmitted()
    {
        PeerSync ps;
        ps.setAppPrefix("BLOG");
        const QString pubkey  = "11223344";
        const QString convoId = ps.convoIdForPubkey(pubkey);
        ps.subscribe(pubkey);

        QSignalSpy spy(&ps, &PeerSync::messageReceived);
        ps.onMessage(convoId, pubkey, "not-hex-!!");
        QCOMPARE(spy.count(), 0);
    }

    void broadcast_noClient_doesNotCrash()
    {
        PeerSync ps;
        ps.setAppPrefix("BLOG");
        ps.setOwnPubkey("aabbccdd");
        ps.broadcast(QByteArray("payload"));
    }

    void subscribe_doubleSubscribe_doesNotDuplicate()
    {
        PeerSync ps;
        ps.setAppPrefix("BLOG");
        const QString pubkey  = "aabbcc11";
        const QString convoId = ps.convoIdForPubkey(pubkey);

        ps.subscribe(pubkey);
        ps.subscribe(pubkey); // second call is a no-op

        QSignalSpy spy(&ps, &PeerSync::messageReceived);
        const QString hex = QString::fromLatin1(QByteArray("x").toHex());
        ps.onMessage(convoId, pubkey, hex);
        QCOMPARE(spy.count(), 1); // exactly one emission
    }
};

QTEST_MAIN(TestPeerSync)
#include "test_peer_sync.moc"
