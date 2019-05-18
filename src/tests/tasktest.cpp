#include <QTest>

#include "taskview.h"
#include "model/task.h"
#include "helpers.h"

class TaskTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testIsRoot();
};

void TaskTest::testIsRoot()
{
    auto *taskView = createTaskView(this, false);

    auto *task1 = taskView->task(taskView->addTask("1"));
    QVERIFY(task1);
    QVERIFY(task1->isRoot());

    auto *task2 = taskView->task(taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1));
    QVERIFY(task2);
    QVERIFY(!task2->isRoot());
}

QTEST_MAIN(TaskTest)

#include "tasktest.moc"
