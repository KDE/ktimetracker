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

    QCOMPARE(taskView->storage()->save(), "");
    QCOMPARE(readTextFile(taskView->storage()->fileUrl().toLocalFile()), readTextFile(QFINDTESTDATA("data/empty.ics")));
}

void StorageTest::testSaveSimpleTree()
{
    auto *taskView = createTaskView(this, true);

    Task* negativeTask = taskView->addTask("negative duration");
    negativeTask->changeTime(-5, taskView->storage()->eventsModel()); // substract 5 minutes

    QCOMPARE(taskView->storage()->save(), "");

    KCalCore::MemoryCalendar::Ptr calendar(new KCalCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    KCalCore::FileStorage fileStorage(calendar, taskView->storage()->fileUrl().toLocalFile(), new KCalCore::ICalFormat());
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
