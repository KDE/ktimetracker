/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QTest>

#include "base/ktimetrackerutility.h"

class UtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFormatTime();
    void testFormatTimeRu();
};

void UtilsTest::testFormatTime()
{
    QLocale::setDefault(QLocale(QLocale::C));

    QCOMPARE(formatTime(-61, false), QStringLiteral("-1:01"));
    QCOMPARE(formatTime(-61, true), QStringLiteral("-1.02")); // -1.01666 -> -1.02

    QCOMPARE(formatTime(7386, false), QStringLiteral("123:06"));
    QCOMPARE(formatTime(7386, true), QStringLiteral("123.10"));
}

void UtilsTest::testFormatTimeRu()
{
    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));

    QCOMPARE(formatTime(0, false), QStringLiteral("0:00"));
    QCOMPARE(formatTime(0, true), QStringLiteral("0,00"));

    QCOMPARE(formatTime(1, false), QStringLiteral("0:01"));
    QCOMPARE(formatTime(1, true), QStringLiteral("0,02")); // 0.01666 -> 0.02

    QCOMPARE(formatTime(2, false), QStringLiteral("0:02"));
    QCOMPARE(formatTime(2, true), QStringLiteral("0,03")); // 0.03333 -> 0.03

    QCOMPARE(formatTime(-61, false), QStringLiteral("-1:01"));
    QCOMPARE(formatTime(-61, true), QStringLiteral("-1,02")); // -1.01666 -> -1.02

    QCOMPARE(formatTime(7386, false), QStringLiteral("123:06"));
    QCOMPARE(formatTime(7386, true), QStringLiteral("123,10"));

    QCOMPARE(formatTime(0.5, false), QStringLiteral("0:01"));
    QCOMPARE(formatTime(0.5, true), QStringLiteral("0,01"));
}

QTEST_GUILESS_MAIN(UtilsTest)

#include "formattimetest.moc"
