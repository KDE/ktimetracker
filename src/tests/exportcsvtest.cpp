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
#include <QTemporaryFile>

#include "taskview.h"
#include "model/task.h"
#include "export/totalsastext.h"
#include "widgets/taskswidget.h"
#include "helpers.h"

class ExportCSVTest : public QObject
{
    Q_OBJECT

private:
    QUrl createTempFile();
    ReportCriteria createRC(ReportCriteria::REPORTTYPE type, bool toClipboard);

private Q_SLOTS:
    void testTotalsEmpty();
    void testTotalsSimpleTree();
    void testTimesSimpleTree();
    void testHistorySimpleTree();
};

QUrl ExportCSVTest::createTempFile()
{
    auto *file = new QTemporaryFile(this);
    if (!file->open()) {
        delete file;
        throw std::runtime_error("1");
    }

    return QUrl::fromLocalFile(file->fileName());
}

ReportCriteria ExportCSVTest::createRC(ReportCriteria::REPORTTYPE type, bool toClipboard)
{
    ReportCriteria rc;
    rc.reportType = type;
    rc.from = QDate::currentDate();
    rc.to = QDate::currentDate();
    rc.decimalMinutes = false;
    rc.sessionTimes = false;
    rc.allTasks = true;
    rc.bExPortToClipBoard = toClipboard;
    rc.delimiter = ";";
    rc.quote = "\"";

    return rc;
}

void ExportCSVTest::testTotalsEmpty()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, false);

    const QString &timeString = QLocale().toString(QDateTime::currentDateTime());
    const QString &expected = QStringLiteral(
        "Task Totals\n%1\n\n"
        "  Time    Task\n----------------------------------------------\nNo tasks.").arg(timeString);
    QCOMPARE(
        totalsAsText(taskView->storage()->tasksModel(), taskView->tasksWidget()->currentItem(), createRC(ReportCriteria::CSVTotalsExport, true)),
        expected);
}

void ExportCSVTest::testTotalsSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, true);

    const QString &timeString = QLocale().toString(QDateTime::currentDateTime());
    const QString &expected = QStringLiteral(
        "Task Totals\n"
         "%1\n"
         "\n"
         "  Time    Task\n"
         "----------------------------------------------\n"
         "  0:08    1\n"
         "   0:03    2\n"
         "  0:07    3\n"
         "----------------------------------------------\n"
         "  0:15 Total").arg(timeString);
    QCOMPARE(
        totalsAsText(taskView->storage()->tasksModel(), taskView->tasksWidget()->currentItem(), createRC(ReportCriteria::CSVTotalsExport, true)),
        expected);
}

void ExportCSVTest::testTimesSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, true);

    const auto &rc = createRC(ReportCriteria::CSVTotalsExport, false);
    const QUrl &url = createTempFile();
    QCOMPARE(taskView->report(rc, url), "");

    const QString &expected = QStringLiteral(
        "\"3\";;0:07;0:07;0:07;0:07\n"
        "\"1\";;0:05;0:05;0:08;0:08\n"
        ";\"2\";0:03;0:03;0:03;0:03\n");
    QCOMPARE(readTextFile(url.path()), expected);
}

void ExportCSVTest::testHistorySimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, true);

    const auto &rc = createRC(ReportCriteria::CSVHistoryExport, false);
    const QUrl &url = createTempFile();
    QCOMPARE(taskView->report(rc, url), "");

    const QString &expected = QStringLiteral(
        "\"Task name\";%1\n"
        "\"1\";0:05\n"
        "\"1->2\";0:03\n"
        "\"3\";0:07\n").arg(QDate::currentDate().toString());
    QCOMPARE(readTextFile(url.path()), expected);
}

QTEST_MAIN(ExportCSVTest)

#include "exportcsvtest.moc"
