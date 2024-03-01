/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QTest>

#include "export/export.h"
#include "export/totalsastext.h"
#include "helpers.h"
#include "model/task.h"
#include "taskview.h"
#include "widgets/taskswidget.h"

class ExportCSVTest : public QObject
{
    Q_OBJECT

private:
    ReportCriteria createRC(ReportCriteria::REPORTTYPE type);

private Q_SLOTS:
    void testTotalsEmpty();
    void testTotalsSimpleTree();
    void testTimesSimpleTree();
    void testHistorySimpleTree();
};

ReportCriteria ExportCSVTest::createRC(ReportCriteria::REPORTTYPE type)
{
    ReportCriteria rc;
    rc.reportType = type;
    rc.from = QDate::currentDate();
    rc.to = QDate::currentDate();
    rc.decimalMinutes = false;
    rc.sessionTimes = false;
    rc.allTasks = true;
    rc.delimiter = QChar::fromLatin1(';');
    rc.quote = QChar::fromLatin1('"');

    return rc;
}

void ExportCSVTest::testTotalsEmpty()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    const QString &timeString = QLocale().toString(QDateTime::currentDateTime());
    const QString &expected = QStringLiteral(
        "Task Totals\n%1\n\n"
        "  Time    Task\n----------------------------------------------\nNo tasks.").arg(timeString);
    QCOMPARE(
        totalsAsText(taskView->storage()->tasksModel(), taskView->tasksWidget()->currentItem(), createRC(ReportCriteria::CSVTotalsExport)),
        expected);
}

void ExportCSVTest::testTotalsSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, true);
    QVERIFY(taskView);

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
        totalsAsText(taskView->storage()->tasksModel(), taskView->tasksWidget()->currentItem(), createRC(ReportCriteria::CSVTotalsExport)),
        expected);
}

void ExportCSVTest::testTimesSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, true);
    QVERIFY(taskView);

    const auto &rc = createRC(ReportCriteria::CSVTotalsExport);
    const QUrl &url = createTempFile(this);
    QVERIFY(!url.isEmpty());
    QString output = exportToString(taskView->storage()->projectModel(), taskView->tasksWidget()->currentItem(), rc);
    QCOMPARE(writeExport(output, url), QString());

    const QString &expected = QStringLiteral(
        "\"3\";;0:07;0:07;0:07;0:07\n"
        "\"1\";;0:05;0:05;0:08;0:08\n"
        ";\"2\";0:03;0:03;0:03;0:03\n");
    QCOMPARE(readTextFile(url.toLocalFile()), expected);
}

void ExportCSVTest::testHistorySimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(this, true);
    QVERIFY(taskView);

    const auto &rc = createRC(ReportCriteria::CSVHistoryExport);
    const QUrl &url = createTempFile(this);
    QString output = exportToString(taskView->storage()->projectModel(), taskView->tasksWidget()->currentItem(), rc);
    QCOMPARE(writeExport(output, url), QString());

    const QString &expected = QStringLiteral(
        "\"Task name\";%1\n"
        "\"1\";0:05\n"
        "\"1->2\";0:03\n"
        "\"3\";0:07\n").arg(QDate::currentDate().toString());
    QCOMPARE(readTextFile(url.toLocalFile()), expected);
}

QTEST_MAIN(ExportCSVTest)

#include "exportcsvtest.moc"
