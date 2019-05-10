#include <QTest>
#include <QTemporaryFile>

#include "taskview.h"
#include "model/task.h"
#include "export/totalsastext.h"

class ExportCSVTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testTotalsEmpty();
    void testTotalsSimpleTree();
};

static ReportCriteria createRC()
{
    ReportCriteria rc;
    rc.reportType = ReportCriteria::CSVTotalsExport;
    rc.url = QUrl();
    rc.from = QDate();
    rc.to = QDate();
    rc.decimalMinutes = false;
    rc.sessionTimes = false;
    rc.allTasks = true;
    rc.bExPortToClipBoard = true;
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
    QCOMPARE(totalsAsText(taskView, createRC()), expected);
}

void ExportCSVTest::testTotalsSimpleTree()
{
    QLocale::setDefault(QLocale(QLocale::C));

    auto *taskView = new TaskView();
    QTemporaryFile icsFile;
    QVERIFY(icsFile.open());
    taskView->storage()->load(taskView, QUrl::fromLocalFile(icsFile.fileName()));

    Task* task1 = taskView->task(taskView->addTask("1"));
    QVERIFY(task1);
    Task* task2 = taskView->task(taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1));
    QVERIFY(task2);
    Task* task3 = taskView->task(taskView->addTask("3"));
    QVERIFY(task3);

    task1->changeTime(5, nullptr); // add 5 minutes
    task2->changeTime(3, nullptr); // add 3 minutes
    task3->changeTime(7, nullptr); // add 7 minutes

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
    QCOMPARE(totalsAsText(taskView, createRC()), expected);
}

QTEST_MAIN(ExportCSVTest)

#include "exportcsvtest.moc"
