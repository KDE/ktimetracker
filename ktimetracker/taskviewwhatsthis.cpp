//
// C++ Implementation: taskviewwhatsthis
//
// Description: 
// This is a subclass of QWhatsThis, specially adapted for karm's taskview.
//
// Author: Thorsten Staerk <thorsten@staerk.de>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "taskviewwhatsthis.h"
#include <kdebug.h>
#include <k3listview.h>
#include <klocale.h>
#include <Q3Header>
#include <taskview.h>

TaskViewWhatsThis::TaskViewWhatsThis( QWidget* qw )
 : Q3WhatsThis( qw )
{
  _listView=(TaskView *) qw;
}

TaskViewWhatsThis::~TaskViewWhatsThis()
{
}

QString TaskViewWhatsThis::text ( const QPoint & pos )
{
  QString desc = QString();
  kDebug(5970) << "entering TaskViewWhatsThis::text" << endl;
  int logiCol = _listView->mapToLogiCal(_listView->header()->cellAt(pos.x())); // logical Column
  kDebug(5970) << "logical column is " << logiCol << endl;
  if ( logiCol == 0 ) 
  {
    desc=i18n("Task Name shows the name of a task or subtask you are working on.");
  }
  else
  {
    desc=i18n("Session time: Time for this task since you chose \"Start New Session\".\nTotal Session time: Time for this task and all its subtasks since you chose \"Start New Session\".\nTime: Overall time for this task.\nTotal Time: Overall time for this task and all its subtasks.");
  }
  return desc;
}
