#include <QTest>
#include <QTemporaryFile>

#include "taskview.h"
#include "model/task.h"
#include "export/totalsastext.h"
#include "helpers.h"

class ExportCSVTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testTotalsEmpty();
    void testTotalsSimpleTree();
    void testTimesSimpleTree();
    void testHistorySimpleTree();
};

static ReportCriteria createRC(ReportCriteria::REPORTTYPE type, bool toClipboard)
{
    QTemporaryFile file;
    if (!file.open()) {
        throw std::runtime_error("1");
    }

    ReportCriteria rc;
    rc.reportType = type;
    rc.url = QUrl::fromLocalFile(file.fileName());
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

    auto *taskView = new TaskView();

    const QString &timeString = QLocale().toString(QDateTime::currentDateTime());
    const QString &expected = QStringLiteral(
        "Task Totals\n%1\n\n"
        "  Time    Task\n----------------------------------------------\nNo tasks.").arg(timeString);
    QCOMPARE(
        totalsAsText(taskView->tasksModel(), taskView->currentItem(), createRC(ReportCriteria::CSVTotalsExport, true)),
        expected);
}

void ExportCSVTest::testTotalsSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(true);

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
        totalsAsText(taskView->tasksModel(), taskView->currentItem(), createRC(ReportCriteria::CSVTotalsExport, true)),
        expected);
}

void ExportCSVTest::testTimesSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(true);

    const auto &rc = createRC(ReportCriteria::CSVTotalsExport, false);
    QCOMPARE(taskView->report(rc), "");

    const QString &expected = QStringLiteral(
        "\"3\";;0:07;0:07;0:07;0:07\n"
        "\"1\";;0:05;0:05;0:08;0:08\n"
        ";\"2\";0:03;0:03;0:03;0:03\n");
    QFile file(rc.url.path());
    QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
    QTextStream in(&file);
    QCOMPARE(in.readAll(), expected);
}

void ExportCSVTest::testHistorySimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = createTaskView(true);

    const auto &rc = createRC(ReportCriteria::CSVHistoryExport, false);
    QCOMPARE(taskView->report(rc), "");

    const QString &expected = QStringLiteral(
        "\"Task name\";%1\n"
        "\"1\";0:05\n"
        "\"1->2\";0:03\n"
        "\"3\";0:07\n").arg(QDate::currentDate().toString());
    QFile file(rc.url.path());
    QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
    QTextStream in(&file);
    QCOMPARE(in.readAll(), expected);
}

QTEST_MAIN(ExportCSVTest)

#include "exportcsvtest.moc"
