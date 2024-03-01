/*
    SPDX-FileCopyrightText: 2004 Thorsten Staerk <dev@staerk.de>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/*

this class is here to import tasks from a planner project file to ktimetracker.
the import shall not be limited to ktimetracker (kPlaTo sends greetings)
it imports planner's top-level-tasks on the same level-depth as currentItem.
if there is no currentItem, planner's top-level-tasks will become top-level-tasks in ktimetracker.
it imports as well the level-depth of each task, as its name, as its percent-complete.
test cases:
 - deleting all tasks away, then import!
 - having started with an empty ics, import!
 - with currentItem being a top-level-task, import!
 - with currentItem being a subtask, import!
*/

#include "plannerparser.h"

#include "model/task.h"

PlannerParser::PlannerParser(ProjectModel *projectModel, Task *currentTask)
    : m_withinTasks(false)
    , m_projectModel(projectModel)
    , m_task(nullptr)
    , m_level(0)
{
    // if there is a task one level above currentItem, make it the father of all imported tasks. Set level accordingly.
    // import as well if there a no task in the taskview as if there are.
    // if there are, put the top-level tasks of planner on the same level as currentItem.
    // So you have the chance as well to have the planner tasks at top-level as at a whatever-so-deep sublevel.

    qDebug() << "entering constructor to import planner tasks";
    if (currentTask && currentTask->parentTask()) {
        m_task = currentTask->parentTask();
        m_level = 1;
    }
}

bool PlannerParser::startDocument()
{
    m_withinTasks = false; // becomes true as soon as parsing occurs <tasks>
    return true;
}

bool PlannerParser::startElement(const QString & /*namespaceURI*/,
                                 const QString & /*localName*/,
                                 const QString &qName,
                                 const QXmlAttributes &att)
{
    qDebug() << "entering function";
    QString taskName;
    int taskComplete = 0;

    // only <task>s within <tasks> are processed
    if (qName == QString::fromLatin1("tasks")) {
        m_withinTasks = true;
    }

    if (qName == QString::fromLatin1("task") && m_withinTasks) {
        // find out name and percent-complete
        for (int i = 0; i < att.length(); ++i) {
            if (att.qName(i) == QStringLiteral("name")) {
                taskName = att.value(i);
            }

            if (att.qName(i) == QStringLiteral("percent-complete")) {
                taskComplete = att.value(i).toInt();
            }
        }

        // at the moment, task is still the old task or the old father task (if an endElement occurred) or not existing (if the
        // new task is a top-level-task). Make task the parenttask, if existing.
        DesktopList dl;
        if (m_level++ > 0) {
            Task *parentTask = m_task;
            m_task = new Task(taskName, QString(), 0, 0, dl, m_projectModel, parentTask);
        } else {
            m_task = new Task(taskName, QString(), 0, 0, dl, m_projectModel, nullptr);
            qDebug() << "added" << taskName;
        }
        m_task->setPercentComplete(taskComplete);
    }

    return true;
}

bool PlannerParser::endElement(const QString & /*namespaceURI*/, const QString & /*localName*/, const QString &qName)
{
    // only <task>s within <tasks> increased level, so only decrease for <task>s within <tasks>
    if (m_withinTasks) {
        if (qName == QStringLiteral("task") && m_level-- >= 0) {
            m_task = m_task->parentTask();
        }

        if (qName == QStringLiteral("tasks")) {
            m_withinTasks = false;
        }
    }

    return true;
}
