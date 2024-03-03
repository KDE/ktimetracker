/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QTest>

#include "dialogs/historydialog.h"
#include "model/eventsmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"
#include "base/taskview.h"

#include "helpers.h"

class TaskTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProperties();
    void testRefreshTimes();
    void testDurationFromEvent();
    void testDeleteRunning();
};

void TaskTest::testProperties()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    auto *task1 = taskView->addTask(QStringLiteral("1"));
    QVERIFY(task1);
    QVERIFY(task1->isRoot());
    QCOMPARE(task1->depth(), 0);

    auto *task2 = taskView->addTask(QStringLiteral("2"), QString(), 0, 0, QVector<int>(0, 0), task1);
    QVERIFY(task2);
    QVERIFY(!task2->isRoot());
    QCOMPARE(task2->depth(), 1);
}

void TaskTest::testRefreshTimes()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    auto *task1 = taskView->addTask(QStringLiteral("1"));
    auto *task2 = taskView->addTask(QStringLiteral("2"), QString(), 0, 0, QVector<int>(0, 0), task1);
    task2->changeTime(5, taskView->storage()->eventsModel());

    QCOMPARE(5, task2->time());
    QCOMPARE(5, task2->sessionTime());

    QCOMPARE(taskView->storage()->save(), QString());

    // Open saved file again
    auto *taskView2 = new TaskView();
    taskView2->load(taskView->storage()->fileUrl());

    taskView2->reFreshTimes();

    auto *task2Mirror = taskView2->storage()->tasksModel()->taskByUID(task2->uid());
    QCOMPARE(5, task2Mirror->time());
    QCOMPARE(5, task2Mirror->sessionTime());
}

void TaskTest::testDurationFromEvent()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    auto *task = taskView->addTask(QStringLiteral("1"));
    task->changeTime(1, taskView->storage()->eventsModel());
    QCOMPARE(task->time(), 1);

    // Simulate increment of the hour number of event in "Edit History" dialog.
    // Such editing operation loses sub-second precision.
    auto *event = taskView->storage()->eventsModel()->eventsForTask(task)[0];
    QString endDateString = event->dtEnd().addSecs(3600).toString(HistoryDialog::dateTimeFormat);
    event->setDtEnd(QDateTime::fromString(endDateString, HistoryDialog::dateTimeFormat));

    // Duration must become 61 minutes
    taskView->reFreshTimes();
    QCOMPARE(task->time(), 61);
}

void TaskTest::testDeleteRunning()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    auto *task = taskView->addTask(QStringLiteral("1"));
    taskView->startTimerForNow(task);

    taskView->deleteTaskBatch(task);
}

QTEST_MAIN(TaskTest)

#include "tasktest.moc"
