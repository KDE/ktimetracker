#include <qstack.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <qlistbox.h>
#include <qfileinfo.h>
#include <qlayout.h>

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kmessagebox.h>

#include "task.h"
#include "karm.h"
#include "adddlg.h"
#include "idle.h"
#include "preferences.h"

#define T_LINESIZE 1023

Karm::Karm( QWidget *parent, const char *name )
  : QListView( parent, name )
{
  _preferences = Preferences::instance();
  
	connect(this, SIGNAL(doubleClicked(QListViewItem *)), 
          this, SLOT(changeTimer(QListViewItem *)));

	addColumn(i18n("Task Name"));
  addColumn(i18n("Session Time"));
	addColumn(i18n("Total Time"));
	setAllColumnsShowFocus(true);

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect(_minuteTimer, SIGNAL(timeout()), this, SLOT(minuteUpdate()));
  _minuteTimer->start(1000 * secsPerMinutes);

  // Set up the idle detection.
  _idleTimer = new IdleTimer(_preferences->idlenessTimeout());
  connect(_idleTimer, SIGNAL(extractTime(int)), this, SLOT(extractTime(int)));
  connect(_idleTimer, SIGNAL(stopTimer()), this, SLOT(stopAllTimers()));
  connect(_preferences, SIGNAL(idlenessTimeout(int)), _idleTimer, SLOT(setMaxIdle(int)));
  connect(_preferences, SIGNAL(detectIdleness(bool)), _idleTimer, SLOT(toggleOverAllIdleDetection(bool)));
  
  // Setup auto save timer
  _autoSaveTimer = new QTimer(this);
  connect(_preferences, SIGNAL(autoSave(bool)), this, SLOT(autoSaveChanged(bool)));
  connect(_preferences, SIGNAL(autoSavePeriod(int)), 
          this, SLOT(autoSavePeriodChanged(int)));
  connect(_autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));
}

Karm::~Karm()
{
  save();
}

void Karm::load()
{
  QFileInfo info;
  QString fname = _preferences->saveFile();
  info.setFile(fname);

  if( !info.exists() ) 
    return;

  QFile file( fname );
  if( !file.open( IO_ReadOnly ) )
    return;

  QString line;

  QStack<Task> stack;
  Task *task;
  
  while( !file.atEnd() ) {
    if ( file.readLine( line, T_LINESIZE ) == 0 )
      break;

    long minutes;
    int level;
    QString name;
    
    if (!parseLine(line, &minutes, &name, &level))
      continue;
    
    unsigned int stackLevel = stack.count();
    for (unsigned int i = level; i<=stackLevel ; i++) {
      stack.pop();
    }
    
    if (level == 1) {
      task = new Task(name, minutes, 0, this);
    }
    else {
      Task *parent = stack.top();
      task = new Task(name, minutes, 0, parent);
      setRootIsDecorated(true);
      parent->setOpen(true);
    }
    stack.push(task);
  }
}

bool Karm::parseLine(QString line, long *time, QString *name, int *level)
{
  if (line.find('#') == 0) {
    // A comment line
    return false;
  }
		
  int index = line.find('\t');
  if (index == -1) {
    // This doesn't seem like a valid record
    return false;
  }

  QString levelStr = line.left(index);
  QString rest = line.remove(0,index+1);
  
  index = rest.find('\t');
  if (index == -1) {
    // This doesn't seem like a valid record
    return false;
  }

  QString timeStr = rest.left(index);
  *name = rest.remove(0,index+1);
		
  bool ok;
  *time = timeStr.toLong(&ok);
		
  if (!ok) {
    // the time field was not a number
    return false;
  }
  *level = levelStr.toInt(&ok);
  if (!ok) {
    // the time field was not a number
    return false;
  }
  return true;
}

void Karm::save()
{
  QString fname =  _preferences->saveFile();
  FILE *file = fopen( QFile::encodeName(fname), "w" );

  if( file == 0 ) {
		QString msg = i18n
		  ("There was an error trying to save your data file.\n"
		   "Time accumulated this session will NOT be saved!\n");
		KMessageBox::error(0, msg );
    return;
	}

  fputs( "# Karm save data\n", file );	// file comment
  for (QListViewItem *child =firstChild(); child; child = child->nextSibling()) {
    writeTaskToFile(file, child, 1);
  }
  
  fclose( file );
}

void Karm::writeTaskToFile(FILE *file, QListViewItem *item, int level)
{
  Task *task = (Task *) item;
  fprintf(file,"%d\t%ld\t%s\n", level, task->totalTime(), task->name());

  QListViewItem *child;
  for (child=item->firstChild(); child; child=child->nextSibling()) {
    writeTaskToFile(file, child, level+1);
  }
}

void Karm::startTimer()
{
  Task *item = ((Task *) currentItem());
  if (item != 0 && activeTasks.findRef(item) == -1) {
    _idleTimer->startIdleDetection();
    item->setRunning(true);
    activeTasks.append(item);
  }
}

void Karm::stopAllTimers() {
  for(unsigned int i=0; i<activeTasks.count();i++) {
    activeTasks.at(i)->setRunning(false);
  }
  _idleTimer->stopIdleDetection();
  activeTasks.clear();
}


void Karm::stopCurrentTimer()
{
  Task *item = ((Task *) currentItem());
  if (item != 0 && activeTasks.findRef(item) != -1) {
    activeTasks.removeRef(item);
    item->setRunning(false);
    if (activeTasks.count()== 0) 
      _idleTimer->stopIdleDetection();    
  }
}

void Karm::changeTimer(QListViewItem *)
{
  Task *item = ((Task *) currentItem());
  if (item != 0 && activeTasks.findRef(item) == -1) {
    // Stop all the other timers.
    for (unsigned int i=0; i<activeTasks.count();i++) {
      (activeTasks.at(i))->setRunning(false);
    }
    activeTasks.clear();

    // Start the new timer.
    startTimer();
  }
  else {
    stopCurrentTimer();
  }
}

void Karm::minuteUpdate()
{
  addTimeToActiveTasks(1);
  if (activeTasks.count() != 0)
    emit(timerTick());
}

void Karm::addTimeToActiveTasks(int minutes) 
{
  for(unsigned int i=0; i<activeTasks.count();i++) {
    Task *task = activeTasks.at(i);
    QListViewItem *item = task;
    while (item) {
      ((Task *) item)->incrementTime(minutes);
      item = item->parent();
    }
  }
}

void Karm::newTask()
{
  newTask(i18n("New Task"), 0);
}

void Karm::newTask(QString caption, QListViewItem *parent)
{
  AddTaskDialog *dialog = new AddTaskDialog(caption);
	int result = dialog->exec();

  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }
    Task *task;
    if (parent == 0)
      task = new Task(taskName, dialog->totalTime(), dialog->sessionTime(), this);
    else
      task = new Task(taskName, dialog->totalTime(), dialog->sessionTime(), parent);

    setCurrentItem(task);
    setSelected(task, true);
  }
  delete dialog;
}

void Karm::newSubTask()
{
  QListViewItem *item = currentItem();
  newTask(i18n("New Sub Task"), item);
  item->setOpen(true);
  setRootIsDecorated(true);
}

void Karm::editTask()
{
	Task *task = (Task *) currentItem();
	if (!task)
    return;

  AddTaskDialog *dialog = new AddTaskDialog(i18n("Edit Task"));
  dialog->setTask(QString::fromLatin1(task->name()),
                  task->totalTime(),
                  task->sessionTime());  
	int result = dialog->exec();
  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");   
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }
    task->setName(taskName);
    
    // update session time as well if the time was changed
    long totalDiff = dialog->totalTime() - task->totalTime();
    long sessionDiff = dialog->sessionTime() - task->sessionTime();
    long difference = totalDiff + sessionDiff  ;
    if( difference != 0 ) {
      emit sessionTimeChanged( difference );
    }
    task->setTotalTime( dialog->totalTime() + sessionDiff);
    task->setSessionTime( dialog->sessionTime() );

    // Update the parents for this task.
    QListViewItem *item = ((QListViewItem *)task)->parent();
    while (item) {
      Task *parrentTask = (Task *) item;
      parrentTask->setTotalTime(parrentTask->totalTime()+difference);
      parrentTask->setSessionTime(parrentTask->sessionTime()+sessionDiff);
      item = item->parent();
    }
    
  }
  delete dialog;
}

void Karm::deleteTask()
{
  Task *item = ((Task *) currentItem());
  if (item == 0) {
    KMessageBox::information(0,i18n("No task selected"));
    return;
  }
	
  int response;
  if (item->childCount() == 0) {
    response = KMessageBox::questionYesNo(0,
                                          i18n( "Are you sure you want to delete the task named\n\"%1\"")
                                          .arg(QString::fromLatin1(item->name())),
                                          i18n( "Deleting Task"));
  }
  else {
    response = KMessageBox::questionYesNo(0,
                                          i18n( "Are you sure you want to delete the task named\n\"%1\"\n"
                                                "NOTE: all its subtasks will also be deleted!")
                                          .arg(QString::fromLatin1(item->name())),
                                          i18n( "Deleting Task"));
  }

	if (response == KMessageBox::Yes) {

    // Remove chilren from the active set of tasks.
    stopChildCounters(item);

    // Stop idle detection if no more counters is running
    if (activeTasks.count() == 0) {
      _idleTimer->stopIdleDetection();
    }

		delete item;

    // remove root decoration if there is no more children.
    bool anyChilds = false;
    for(QListViewItem *child=firstChild(); child; child=child->nextSibling()) {
      if (child->childCount() != 0) {
        anyChilds = true;
        break;
      }
    }
    if (!anyChilds) {
      setRootIsDecorated(false);
    }
	}
}

void Karm::stopChildCounters(Task *item)
{
  for (QListViewItem *child=item->firstChild(); child; child=child->nextSibling()) {
    stopChildCounters((Task *)child);
  }
  activeTasks.removeRef(item);
}


void Karm::extractTime(int minutes) 
{
  addTimeToActiveTasks(-minutes);
  emit(sessionTimeChanged(-minutes));
}

void Karm::autoSaveChanged(bool on)
{
  if (on) {
    if (!_autoSaveTimer->isActive()) {
      _autoSaveTimer->start(_preferences->autoSavePeriod()*1000*secsPerMinutes);
    }
  }
  else {
    if (_autoSaveTimer->isActive()) {
      _autoSaveTimer->stop();
    }
  }
}

void Karm::autoSavePeriodChanged(int /*minutes*/) 
{
  autoSaveChanged(_preferences->autoSave());
}

