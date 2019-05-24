/*
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QTest>

#include "ktimetrackerutility.h"

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

    QCOMPARE(formatTime(-61, false), "-1:01");
    QCOMPARE(formatTime(-61, true), "-1.02"); // -1.01666 -> -1.02

    QCOMPARE(formatTime(7386, false), "123:06");
    QCOMPARE(formatTime(7386, true), "123.10");
}

void UtilsTest::testFormatTimeRu()
{
    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));

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

    QCOMPARE(formatTime(0.5, false), "0:01");
    QCOMPARE(formatTime(0.5, true), "0,01");
}

QTEST_GUILESS_MAIN(UtilsTest)

#include "formattimetest.moc"
