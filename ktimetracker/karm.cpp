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
#include "karm.moc"
#include "idle.h"
#include "preferences.h"

#define T_LINESIZE 1023

Karm::Karm( QWidget *parent, const char *name )
  : QListView( parent, name )
{
  _addDlg  = 0;
  _editDlg = 0;  
  
  _preferences = Preferences::instance();
  
	connect(this, SIGNAL(doubleClicked(QListViewItem *)), 
          this, SLOT(editTask(QListViewItem *)));

	addColumn(i18n("Total Time"));
  addColumn(i18n("Session Time"));
	addColumn(i18n("Task Name"));
	setAllColumnsShowFocus(true);

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect(_minuteTimer, SIGNAL(timeout()), this, SLOT(minuteUpdate()));

  // Set up the idle detection.
  _idleTimer = new IdleTimer(_preferences->idlenessTimeout());
  connect(_idleTimer, SIGNAL(extractTime(int)), this, SLOT(extractTime(int)));
  connect(_idleTimer, SIGNAL(stopTimer()), this, SLOT(stopClock()));
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
  info.setFile(_preferences->saveFile() );

  if( info.exists() ) 
  {
    readFromFile( info.filePath() );
    return;	
  }
}

void Karm::readFromFile( const QString &fname ) 
{
  if ( fname.isNull() )
    return;

  QFile file( fname );

  if( !file.open( IO_ReadOnly ) ) {
    // file open error
    emit fileError( fname );
    return;
  }

  QString line;

  while( !file.atEnd() ) {
    if ( file.readLine( line, T_LINESIZE ) == 0 )
      break;

    if (line.find('#') == 0) {
      // A comment line
      continue;
    }
		
    int index = line.find('\t');
    if (index == -1) {
      // This doesn't seem like a valid record
      continue;
    }
		
    QString time = line.left(index);
    QString name = line.remove(0,index);
		
    bool ok;
    long minutes = time.toLong(&ok);
		
    if (!ok) {
      // the time field was not a number
      continue;
    }
    new Task(name, minutes, 0, this);
  }
}

void Karm::save()
{
	if( !writeToFile( _preferences->saveFile() ) ) {
		QString msg = i18n
		  ("There was an error trying to save your data file.\n"
		   "Time accumulated this session will NOT be saved!\n");
		KMessageBox::error(0, msg );
	}
}

bool Karm::writeToFile(const QString &fname)
{
  if( fname.isNull() ) return 0;

  FILE *file = fopen( QFile::encodeName(fname), "w" );

  if( file == 0 ) {
    emit fileError( fname );
    return FALSE;
  }

  fputs( "# Karm save data\n", file );	// file comment
  for (QListViewItem *child =firstChild(); child; child = child->nextSibling()) {
    Task *task = (Task *) child;
    fprintf(file, "%ld\t%s\n", 
	    task->totalTime(),
	    task->name());
  }
  fclose( file );

  emit dataChanged();

  return TRUE;
}

void Karm::stopClock()
{
  if(_minuteTimer->isActive()) {
    _minuteTimer->stop();
    _idleTimer->stopIdleDetection();    
    emit(timerStopped());
  }
}

void Karm::startClock()
{
  if( !_minuteTimer->isActive() && (childCount() > 0) && (currentItem() != 0))
  {
    _minuteTimer->start(1000 * secsPerMinutes);
    _idleTimer->startIdleDetection();
    emit(timerStarted());
  }
}

void Karm::minuteUpdate()
{
  ((Task *) currentItem())->incrementTime( 1 );
  emit(timerTick());
}

void Karm::newTask()
{
	if( _addDlg == 0 ) 
	{
		_addDlg = new AddTaskDialog( topLevelWidget(), 0, true );
		_addDlg->setCaption(i18n("New Task"));
		connect( _addDlg, SIGNAL( finished( bool ) ), 
			 this, SLOT( createNewTask( bool ) ) );
	}
	_addDlg->show();
}

void Karm::createNewTask( bool retVal )
{
	if( _addDlg == 0 ) {
		warning("Karm::createNewTask called and there's no dialog!" );
		return;
	}
	
	if( retVal && !_addDlg->taskName().isEmpty() ) {
		// create the new task
		Task *task = new Task(_addDlg->taskName(),_addDlg->totalTime(), _addDlg->sessionTime(), this);
		setCurrentItem(task);
		setSelected(task, true);
	}
	
	// dispose of the dialog
	delete _addDlg;
	_addDlg = 0;
}

void Karm::editTask()
{
	QListViewItem *item = currentItem();
	if (item)
		editTask(item);
}

void Karm::editTask(QListViewItem *element)
{
	Task *task= (Task *)element;
	
	if( _editDlg == 0 ) 
	{
		_editDlg = new AddTaskDialog( topLevelWidget(), 0, true );
		_editDlg->setCaption(i18n("Edit Task"));
		_editDlg->setTask( QString::fromLatin1(task->name()),
				   task->totalTime(),
				   task->sessionTime());  
		connect( _editDlg, SIGNAL( finished( bool ) ),
			 this, SLOT( updateExistingTask( bool ) ) );
	}
	_editDlg->show();
}

void Karm::updateExistingTask( bool retVal )
{
	if( _editDlg == 0 ) {
		warning("Karm::updateExistingTask called and there's no dialog!" );
		return;
	}
	
	if( retVal && !_editDlg->taskName().isEmpty() ) {
		Task *task = (Task *) currentItem();
		task->setName( _editDlg->taskName() );

		// update session time as well if the time was changed
    long totalDiff = _editDlg->totalTime() - task->totalTime();
    long sessionDiff = _editDlg->sessionTime() - task->sessionTime();
    long difference = totalDiff + sessionDiff  ;
		if( difference != 0 ) {
		  emit sessionTimeChanged( difference );
		}

		task->setTotalTime( _editDlg->totalTime() + sessionDiff);
		task->setSessionTime( _editDlg->sessionTime() );
	}
	
	delete _editDlg;
	_editDlg = 0;
}

void Karm::deleteTask()
{
	if( childCount() == 0 )
		return;
	
	int response = KMessageBox::questionYesNo(0,
		i18n( "Are you sure you want to delete this task?" ),
		i18n( "Deleting Task"));

	if (response == KMessageBox::Yes) {
		delete currentItem();
		
		if( childCount() == 0 )
			stopClock();
	}
}


void Karm::extractTime(int minutes) 
{
  Task *task = (Task *) currentItem();
  task->decrementTime(minutes);
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
