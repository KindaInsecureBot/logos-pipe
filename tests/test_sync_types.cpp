#include <QtTest>
#include <QCryptographicHash>
#include "sync_types.h"

class TestSyncTypes : public QObject {
    Q_OBJECT
private slots:
    void deriveChannelId_knownVector()
    {
        // Manually compute SHA-256("λBLOG:post-001") and compare.
        const QByteArray input = QStringLiteral("λBLOG:post-001").toUtf8();
        const QString expected = QString::fromLatin1(
            QCryptographicHash::hash(input, QCryptographicHash::Sha256).toHex());
        QCOMPARE(LogosSync::deriveChannelId("BLOG", "post-001"), expected);
    }

    void deriveChannelId_differentAppsDiffer()
    {
        const QString a = LogosSync::deriveChannelId("BLOG",  "doc-1");
        const QString b = LogosSync::deriveChannelId("NOTES", "doc-1");
        QVERIFY(a != b);
    }

    void deriveChannelId_differentIdsDiffer()
    {
        const QString a = LogosSync::deriveChannelId("WIKI", "page-1");
        const QString b = LogosSync::deriveChannelId("WIKI", "page-2");
        QVERIFY(a != b);
    }

    void deriveChannelId_isSha256Hex()
    {
        const QString id = LogosSync::deriveChannelId("DOCS", "index");
        // SHA-256 hex is exactly 64 characters.
        QCOMPARE(id.length(), 64);
        QVERIFY(QRegularExpression("^[0-9a-f]{64}$").match(id).hasMatch());
    }

    void deriveConvoId_knownVector()
    {
        const QString pubkey = "aabbccdd";
        const QByteArray input = QStringLiteral("BLOG-channel:%1").arg(pubkey).toUtf8();
        const QString expected = QString::fromLatin1(
            QCryptographicHash::hash(input, QCryptographicHash::Sha256).toHex());
        QCOMPARE(LogosSync::deriveConvoId("BLOG", pubkey), expected);
    }

    void deriveConvoId_differentPrefixDiffer()
    {
        const QString pubkey = "deadbeef";
        const QString a = LogosSync::deriveConvoId("BLOG",  pubkey);
        const QString b = LogosSync::deriveConvoId("NOTES", pubkey);
        QVERIFY(a != b);
    }

    void appConstants_defined()
    {
        QVERIFY(QLatin1String(LogosSync::Apps::BLOG)  == "BLOG");
        QVERIFY(QLatin1String(LogosSync::Apps::NOTES) == "NOTES");
        QVERIFY(QLatin1String(LogosSync::Apps::WIKI)  == "WIKI");
        QVERIFY(QLatin1String(LogosSync::Apps::DOCS)  == "DOCS");
    }
};

QTEST_MAIN(TestSyncTypes)
#include "test_sync_types.moc"
