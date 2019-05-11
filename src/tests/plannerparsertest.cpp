#include <QTest>

#include "taskview.h"
#include "model/task.h"
#include "helpers.h"

class PlannerParserTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testEmpty();
    void testAtTopLevel();
    void testAtSubTask();
};

void PlannerParserTest::testEmpty()
{
    auto *taskView = new TaskView();
    taskView->importPlanner(QFINDTESTDATA("data/kitchen.planner"));

    auto *model = taskView->tasksModel();

    QCOMPARE(model->getAllItems().size(), 21);

    QCOMPARE(model->topLevelItemCount(), 4);
    QCOMPARE(dynamic_cast<Task*>(model->topLevelItem(0))->name(), "Initiation");

    auto *closureTask = dynamic_cast<Task*>(model->topLevelItem(3));
    QCOMPARE(closureTask->name(), "Closure");
    QCOMPARE(closureTask->childCount(), 3);
}

void PlannerParserTest::testAtTopLevel()
{
    auto *taskView = createTaskView();
    Task* task1 = taskView->task(taskView->addTask("1"));
    QVERIFY(task1);
    taskView->setCurrentIndex(taskView->tasksModel()->index(task1, 0));

    taskView->importPlanner(QFINDTESTDATA("data/kitchen.planner"));

    auto *model = taskView->tasksModel();

    QCOMPARE(model->getAllItems().size(), 22);

    QCOMPARE(model->topLevelItemCount(), 5);
    QCOMPARE(dynamic_cast<Task*>(model->topLevelItem(0))->name(), "1");
    QCOMPARE(dynamic_cast<Task*>(model->topLevelItem(1))->name(), "Initiation");

    auto *closureTask = dynamic_cast<Task*>(model->topLevelItem(4));
    QCOMPARE(closureTask->name(), "Closure");
    QCOMPARE(closureTask->childCount(), 3);
}

void PlannerParserTest::testAtSubTask()
{
    auto *taskView = createTaskView();
    Task* task1 = taskView->task(taskView->addTask("1"));
    QVERIFY(task1);
    Task* task2 = taskView->task(taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1));
    QVERIFY(task2);
    taskView->setCurrentIndex(taskView->tasksModel()->index(task2, 0));

    taskView->importPlanner(QFINDTESTDATA("data/kitchen.planner"));

    auto *model = taskView->tasksModel();

    QCOMPARE(model->getAllItems().size(), 23);

    QCOMPARE(model->topLevelItemCount(), 1);
    QCOMPARE(task1->childCount(), 5);
    QCOMPARE(dynamic_cast<Task*>(task1->child(0))->name(), "2");
    QCOMPARE(dynamic_cast<Task*>(task1->child(1))->name(), "Initiation");

    auto *closureTask = dynamic_cast<Task*>(task1->child(4));
    QCOMPARE(closureTask->name(), "Closure");
    QCOMPARE(closureTask->childCount(), 3);
}

QTEST_MAIN(PlannerParserTest)

#include "plannerparsertest.moc"
