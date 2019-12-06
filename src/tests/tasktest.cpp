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

#include "dialogs/historydialog.h"
#include "model/eventsmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"
#include "taskview.h"

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

    auto *task1 = taskView->addTask("1");
    QVERIFY(task1);
    QVERIFY(task1->isRoot());
    QCOMPARE(task1->depth(), 0);

    auto *task2 = taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1);
    QVERIFY(task2);
    QVERIFY(!task2->isRoot());
    QCOMPARE(task2->depth(), 1);
}

void TaskTest::testRefreshTimes()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    auto *task1 = taskView->addTask("1");
    auto *task2 = taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1);
    task2->changeTime(5, taskView->storage()->eventsModel());

    QCOMPARE(5, task2->time());
    QCOMPARE(5, task2->sessionTime());

    taskView->save();

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

    auto *task = taskView->addTask("1");
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

    auto *task = taskView->addTask("1");
    taskView->startTimerForNow(task);

    taskView->deleteTaskBatch(task);
}

QTEST_MAIN(TaskTest)

#include "tasktest.moc"
