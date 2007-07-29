/*
 *     Copyright (C) 2004 by Thorsten Staerk <dev@staerk.de>
 *                   2007 the ktimetracker developers
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

/*

this class is here to import tasks from a planner project file to karm.
the import shall not be limited to karm (kPlaTo sends greetings)
it imports planner's top-level-tasks on the same level-depth as current_item.
if there is no current_item, planner's top-level-tasks will become top-level-tasks in karm.
it imports as well the level-depth of each task, as its name, as its percent-complete.
test cases:
 - deleting all tasks away, then import!
 - having started with an empty ics, import!
 - with current_item being a top-level-task, import!
 - with current_item being a subtask, import!
*/

#include "plannerparser.h"

#include "task.h"
#include "taskview.h"

  PlannerParser::PlannerParser(TaskView * tv)
  // if there is a task one level above current_item, make it the father of all imported tasks. Set level accordingly.
  // import as well if there a no task in the taskview as if there are.
  // if there are, put the top-level tasks of planner on the same level as current_item.
  // So you have the chance as well to have the planner tasks at top-level as at a whatever-so-deep sublevel.
  {
    kDebug() << "entering constructor to import planner tasks" << endl;
    _taskView=tv;
    level=0;
    if (_taskView->current_item()) if (_taskView->current_item()->parent()) 
    {
      task = _taskView->current_item()->parent(); 
      level=1;
    }
  }
  
  bool PlannerParser::startDocument()
  {
    withInTasks=false; // becomes true as soon as parsing occurres <tasks>
    return true;
  }
  
  bool PlannerParser::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& att )
  {
    kDebug() << "entering startElement" << endl;
    QString taskName;
    int     taskComplete=0;
    
    // only <task>s within <tasks> are processed
    if (qName == QString::fromLatin1("tasks")) withInTasks=true;
    if ((qName == QString::fromLatin1("task")) && (withInTasks))
    {
	
      // find out name and percent-complete
      for (int i=0; i<att.length(); i++)
      {
        if (att.qName(i) == QString::fromLatin1("name")) taskName=att.value(i);
        if (att.qName(i)==QString::fromLatin1("percent-complete")) taskComplete=att.value(i).toInt();
      }
    
      // at the moment, task is still the old task or the old father task (if an endElement occurred) or not existing (if the
      // new task is a top-level-task). Make task the parenttask, if existing.
      DesktopList dl;
      if (level++>0) 
      {
        parentTask=task;
        task = new Task(taskName, 0, 0, dl, parentTask);
        task->setUid(_taskView->storage()->addTask(task, parentTask));
      }
      else
      {
        task = new Task(taskName, 0, 0, dl, _taskView);
        kDebug() << "added" << taskName << endl;
        task->setUid(_taskView->storage()->addTask(task, 0));	  
      }
    
      task->setPercentComplete(taskComplete, _taskView->storage());
    }
    return true;
 }
    
  bool PlannerParser::endElement( const QString&, const QString&, const QString& qName)
  {
    // only <task>s within <tasks> increased level, so only decrease for <task>s within <tasks>
    if (withInTasks)
    {
      if (qName=="task")  if (level-->=0) task=task->parent();
      if (qName=="tasks") withInTasks=false;
    }
    return true;
  }

