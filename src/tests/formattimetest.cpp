#include <QTest>
#include <QObject>

#include "ktimetrackerutility.h"

class UtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFormatTime();
};

void UtilsTest::testFormatTime()
{
    QCOMPARE(formatTime(0, false), "0:00");
    QCOMPARE(formatTime(0, true), "0,00");

    QCOMPARE(formatTime(1, false), "0:01");
    QCOMPARE(formatTime(1, true), "0,02"); // 0.01666 -> 0.02

    QCOMPARE(formatTime(2, false), "0:02");
    QCOMPARE(formatTime(2, true), "0,03"); // 0.03333 -> 0.03

    QCOMPARE(formatTime(-61, false), "-1:01");
    QCOMPARE(formatTime(-61, true), "-1,02"); // -1.01666 -> -1.02

    QCOMPARE(formatTime(7386, false), "123:06");
    QCOMPARE(formatTime(7386, true), "123,10");
}

QTEST_GUILESS_MAIN(UtilsTest)

#include "formattimetest.moc"
