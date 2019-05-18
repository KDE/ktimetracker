#include <QTest>
#include <QTemporaryFile>

#include <KCalCore/FileStorage>
#include <KCalCore/ICalFormat>

#include "taskview.h"
#include "model/task.h"
#include "helpers.h"

class StorageTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSaveEmpty();
    void testSaveSimpleTree();
};

void StorageTest::testSaveEmpty()
{
    auto *taskView = createTaskView(this, false);

    QCOMPARE(taskView->storage()->save(taskView), "");
    QCOMPARE(readTextFile(taskView->storage()->fileUrl().path()), readTextFile(QFINDTESTDATA("data/empty.ics")));
}

void StorageTest::testSaveSimpleTree()
{
    auto *taskView = createTaskView(this, true);

    Task* negativeTask = taskView->task(taskView->addTask("negative duration"));
    negativeTask->changeTime(-5, taskView->storage()); // substract 5 minutes

    QCOMPARE(taskView->storage()->save(taskView), "");

    KCalCore::MemoryCalendar::Ptr calendar(new KCalCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    KCalCore::FileStorage fileStorage(calendar, taskView->storage()->fileUrl().path(), new KCalCore::ICalFormat());
    QVERIFY(fileStorage.load());

    auto todos = calendar->rawTodos(KCalCore::TodoSortSummary);
    QCOMPARE(todos.size(), 4);

    QCOMPARE(todos[0]->summary(), "1");
    QCOMPARE(todos[0]->customProperty("ktimetracker", "totalSessionTime"), "5");
    QCOMPARE(todos[0]->customProperty("ktimetracker", "totalTaskTime"), "5");

    QCOMPARE(todos[1]->summary(), "2");
    QCOMPARE(todos[1]->relatedTo(), todos[0]->uid());

    QCOMPARE(todos[3]->summary(), "negative duration");
    QCOMPARE(todos[3]->customProperty("ktimetracker", "totalSessionTime"), "-5");
    QCOMPARE(todos[3]->customProperty("ktimetracker", "totalTaskTime"), "-5");

    auto events = calendar->rawEvents(KCalCore::EventSortSummary);
    QCOMPARE(events.size(), 4);

    QCOMPARE(events[0]->summary(), "1");
    QCOMPARE(events[0]->customProperty("ktimetracker", "duration"), "300");
    QCOMPARE(events[0]->relatedTo(), todos[0]->uid());
    QCOMPARE(events[0]->categoriesStr(), "KTimeTracker");

    QCOMPARE(events[3]->summary(), "negative duration");
    QCOMPARE(events[3]->customProperty("ktimetracker", "duration"), "-300");
    QCOMPARE(events[3]->relatedTo(), todos[3]->uid());
}

QTEST_MAIN(StorageTest)

#include "storagetest.moc"
