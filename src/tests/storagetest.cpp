/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QTest>

#include <KCalendarCore/FileStorage>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include "base/taskview.h"
#include "helpers.h"
#include "model/task.h"

class StorageTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSaveEmpty();
    void testSaveSimpleTree();
    void testLockPath();
};

void StorageTest::testSaveEmpty()
{
    auto *taskView = createTaskView(this, false);
    QVERIFY(taskView);

    QCOMPARE(taskView->storage()->save(), QString());
    QCOMPARE(readTextFile(taskView->storage()->fileUrl().toLocalFile()), readTextFile(QFINDTESTDATA("data/empty.ics")));
}

void StorageTest::testSaveSimpleTree()
{
    auto *taskView = createTaskView(this, true);
    QVERIFY(taskView);

    Task *negativeTask = taskView->addTask(QStringLiteral("negative duration"));
    negativeTask->changeTime(-5, taskView->storage()->eventsModel()); // subtract 5 minutes

    QCOMPARE(taskView->storage()->save(), QString());

    KCalendarCore::MemoryCalendar::Ptr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    KCalendarCore::FileStorage fileStorage(calendar, taskView->storage()->fileUrl().toLocalFile(), new KCalendarCore::ICalFormat());
    QVERIFY(fileStorage.load());

    auto todos = calendar->rawTodos(KCalendarCore::TodoSortSummary);
    QCOMPARE(todos.size(), 4);

    QCOMPARE(todos[0]->summary(), QStringLiteral("1"));
    QCOMPARE(todos[0]->customProperty("ktimetracker", "totalSessionTime"), QStringLiteral("5"));
    QCOMPARE(todos[0]->customProperty("ktimetracker", "totalTaskTime"), QStringLiteral("5"));

    QCOMPARE(todos[1]->summary(), QStringLiteral("2"));
    QCOMPARE(todos[1]->relatedTo(), todos[0]->uid());

    QCOMPARE(todos[3]->summary(), QStringLiteral("negative duration"));
    QCOMPARE(todos[3]->customProperty("ktimetracker", "totalSessionTime"), QStringLiteral("-5"));
    QCOMPARE(todos[3]->customProperty("ktimetracker", "totalTaskTime"), QStringLiteral("-5"));

    auto events = calendar->rawEvents(KCalendarCore::EventSortSummary);
    QCOMPARE(events.size(), 4);

    QCOMPARE(events[0]->summary(), QStringLiteral("1"));
    QCOMPARE(events[0]->customProperty("ktimetracker", "duration"), QStringLiteral("300"));
    QCOMPARE(events[0]->relatedTo(), todos[0]->uid());
    QCOMPARE(events[0]->categoriesStr(), QStringLiteral("KTimeTracker"));

    QCOMPARE(events[3]->summary(), QStringLiteral("negative duration"));
    QCOMPARE(events[3]->customProperty("ktimetracker", "duration"), QStringLiteral("-300"));
    QCOMPARE(events[3]->relatedTo(), todos[3]->uid());
}

void StorageTest::testLockPath()
{
#ifdef Q_OS_WIN
    // SHA-1 of "C:/var/hello.ics"
    const QString expectedSuffix1 = QStringLiteral(
        "/ktimetracker_a1a9ea47d71476676afa881247fcd8ae915788b5_hello.ics."
        "lock");
#else
    // SHA-1 of "/var/hello.ics"
    const QString expectedSuffix1 = QStringLiteral(
        "/ktimetracker_290fa23d3a268915fba5b2b2e5818aec381b7044_hello.ics."
        "lock");
#endif
    QVERIFY(TimeTrackerStorage::createLockFileName(QUrl::fromLocalFile(QStringLiteral("/tmp/../var/hello.ics"))).endsWith(expectedSuffix1));

    // SHA-1 of "https://abc/1/привет world"
    QVERIFY(TimeTrackerStorage::createLockFileName(QUrl(QStringLiteral("https://abc/1/привет%20world")))
                .endsWith(QDir().absoluteFilePath(QStringLiteral("/ktimetracker_7048956ad41735da76ae0da13d116b500753ba9e_"
                                                                 "привет world.lock"))));
}

QTEST_MAIN(StorageTest)

#include "storagetest.moc"
