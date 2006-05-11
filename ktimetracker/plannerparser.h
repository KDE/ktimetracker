//
// C++ Interface: plannerparser
//
// Description:
//
//
// Author: Thorsten Staerk <Thorsten@Staerk.de>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PLANNERPARSER_H
#define PLANNERPARSER_H

/**
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

@author Thorsten Staerk
*/

#include <QtXml>
#include <klocale.h>
#include "taskview.h"
#include "task.h"
#include "karmstorage.h"
#include "kapplication.h"

class PlannerParser : public QXmlDefaultHandler
{
public:

  /**  Stores the active TaskView in this parser. Returns error code (not always, hopefully)  */
  PlannerParser(TaskView * tv);

  /** given by the framework from qxml. Called when parsing the xml-document starts.          */
  bool startDocument();

  /** given by the framework from qxml. Called when the reader occurs an open tag (e.g. \<b\> ) */
  bool startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& att );

  /** given by the framework from qxml. Called when the reader occurs a closed tag (e.g. \</b\> )*/
  bool endElement( const QString&, const QString&, const QString& qName);

private:
  bool withInTasks;     // within <tasks> ?
  TaskView *_taskView;
  Task *task;
  Task *parentTask;
  int level;            // level=1: task is top-level-task
};


#endif
