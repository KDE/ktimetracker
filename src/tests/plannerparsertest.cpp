/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QTest>

#include "helpers.h"
#include "model/task.h"
#include "model/tasksmodel.h"
#include "base/taskview.h"
#include "widgets/taskswidget.h"

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
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    taskView->importPlanner(QFINDTESTDATA("data/kitchen.planner"));

    auto *model = taskView->storage()->tasksModel();

    QCOMPARE(model->getAllItems().size(), 21);

    QCOMPARE(model->topLevelItemCount(), 4);
    QCOMPARE(dynamic_cast<Task *>(model->topLevelItem(0))->name(), QStringLiteral("Initiation"));

    auto *closureTask = dynamic_cast<Task *>(model->topLevelItem(3));
    QCOMPARE(closureTask->name(), QStringLiteral("Closure"));
    QCOMPARE(closureTask->childCount(), 3);
}

void PlannerParserTest::testAtTopLevel()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    Task *task1 = taskView->addTask(QStringLiteral("1"));
    QVERIFY(task1);
    taskView->tasksWidget()->setCurrentIndex(taskView->storage()->tasksModel()->index(task1, 0));

    taskView->importPlanner(QFINDTESTDATA("data/kitchen.planner"));

    auto *model = taskView->storage()->tasksModel();

    QCOMPARE(model->getAllItems().size(), 22);

    QCOMPARE(model->topLevelItemCount(), 5);
    QCOMPARE(dynamic_cast<Task *>(model->topLevelItem(0))->name(), QStringLiteral("1"));
    QCOMPARE(dynamic_cast<Task *>(model->topLevelItem(1))->name(), QStringLiteral("Initiation"));

    auto *closureTask = dynamic_cast<Task *>(model->topLevelItem(4));
    QCOMPARE(closureTask->name(), QStringLiteral("Closure"));
    QCOMPARE(closureTask->childCount(), 3);
}

void PlannerParserTest::testAtSubTask()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    Task *task1 = taskView->addTask(QStringLiteral("1"));
    QVERIFY(task1);
    Task *task2 = taskView->addTask(QStringLiteral("2"), QString(), 0, 0, QVector<int>(0, 0), task1);
    QVERIFY(task2);
    taskView->tasksWidget()->setCurrentIndex(taskView->storage()->tasksModel()->index(task2, 0));

    taskView->importPlanner(QFINDTESTDATA("data/kitchen.planner"));

    auto *model = taskView->storage()->tasksModel();

    QCOMPARE(model->getAllItems().size(), 23);

    QCOMPARE(model->topLevelItemCount(), 1);
    QCOMPARE(task1->childCount(), 5);
    QCOMPARE(dynamic_cast<Task *>(task1->child(0))->name(), QStringLiteral("2"));
    QCOMPARE(dynamic_cast<Task *>(task1->child(1))->name(), QStringLiteral("Initiation"));

    auto *closureTask = dynamic_cast<Task *>(task1->child(4));
    QCOMPARE(closureTask->name(), QStringLiteral("Closure"));
    QCOMPARE(closureTask->childCount(), 3);
}

QTEST_MAIN(PlannerParserTest)

#include "plannerparsertest.moc"
