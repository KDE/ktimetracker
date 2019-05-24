/*
 * Copyright (C) 2004 by Thorsten Staerk <dev@staerk.de>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

#ifndef PLANNERPARSER_H
#define PLANNERPARSER_H

#include <QtXml/QtXml>

class ProjectModel;
class Task;

/**
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

@author Thorsten Staerk
*/
class PlannerParser : public QXmlDefaultHandler
{
public:

    /** Stores the active TaskView in this parser. */
    explicit PlannerParser(ProjectModel *projectModel, Task *currentTask);

    /** Called when parsing the xml-document starts. */
    bool startDocument() override;

    /** Called when the reader occurs an open tag (e.g. \<b\> ) */
    bool startElement(const QString&, const QString&, const QString& qName, const QXmlAttributes& att) override;

    /** Called when the reader occurs a closed tag (e.g. \</b\> )*/
    bool endElement(const QString&, const QString&, const QString& qName) override;

private:
    bool m_withinTasks;     // within <tasks> ?
    ProjectModel *m_projectModel;
    Task* m_task;
    int m_level;            // m_level=1: task is top-level-task
};

#endif
