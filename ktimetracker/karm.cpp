#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <qlistbox.h>
#include <qfileinfo.h>
#include <qlayout.h>

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include <kpanner.h>
#include <ktoolbar.h>

#include "task.h"
#include "karm.h"
#include "adddlg.h"

Karm::Karm( QWidget *parent )
	:	KPanner( parent )
{
	QBoxLayout *layout;
	_timerRunning = FALSE;

	_addDlg  = 0;
	_editDlg = 0;

	_broker = new TaskBroker;

	layout = new QBoxLayout( child0(), QBoxLayout::LeftToRight );
	_timeList = new QListBox( child0() );
	layout->addWidget( _timeList );

	layout = new QBoxLayout( child1(), QBoxLayout::LeftToRight );
	_nameList = new QListBox( child1() );
	layout->addWidget( _nameList );

	_timeStr = new char[50];

	_broker->moveCursor( 0 );

	connect(_nameList, SIGNAL( highlighted(int) ), this, SLOT(moveTo(int)));
	connect(_timeList, SIGNAL( highlighted(int) ), this, SLOT(moveTo(int)));
	setSeparator( 10 );
	startClock();
	
	// Peter Putzer: added KarmName
	KarmName = QString(kapp->getCaption());
}

Karm::~Karm()
{
	save();
	delete _broker;
	delete[] _timeStr;
}

void Karm::load()
{
	QFileInfo info;
	KConfig *config = kapp->getConfig();
	
	config->setGroup( "Karm" );
	
	QString defaultPath( locateLocal("appdata", "karmdata.txt"));
	info.setFile( config->readEntry( "DataPath", defaultPath ) );

	if( info.exists() ) {
		_broker->readFromFile( info.filePath().ascii() );
		fillListBoxes();
		return;	
	}
}

void Karm::save()
{
	KConfig *config = kapp->getConfig();

	config->setGroup("Karm");
	QString defaultPath( locateLocal("appdata", "karmdata.txt"));

	if( !_broker->writeToFile( config->readEntry("DataPath", defaultPath).ascii() ) ) {

               KMsgBox::message(0, i18n( "Karm: Save error" ), 
                       i18n( "There was an error trying to save your data file.\n"
                       "Time accumulated this session will NOT be saved!\n" ),
                       KMsgBox::STOP, i18n( "Oh No!" ) );
	}
}

void Karm::stopClock()
{
	if( _timerRunning ) {
		killTimer( _timerId );
		_timerRunning = FALSE;

		emit timerStopped();
	}
}

void Karm::startClock()
{
	if( !_timerRunning && (_broker->count() > 0)) {
		_timerId = startTimer( 60000 );
		_timerRunning = TRUE;

		emit timerStarted();
	}
}

void Karm::moveTo( int position )
{
	_nameList->setCurrentItem( position );
	_timeList->setCurrentItem( position );
	_broker->moveTo( position );
}

void Karm::fillListBoxes()
{
	int taskCount = _broker->count();

	_timeList->clear();
	_nameList->clear();

	if( taskCount == 0 )
		return;

	_broker->top();

	do {
		Task *task = _broker->currentTask();

		formatTime( _timeStr, task->time() );

		_timeList->insertItem( _timeStr );
		_nameList->insertItem( task->name() );

		_broker->next();

	} while( --taskCount );

	_broker->top();
	_nameList->setCurrentItem( 0 );
	_timeList->setCurrentItem( 0 );
}

void Karm::timerEvent( QTimerEvent *ev )
{
	KPanner::timerEvent( ev );

	if ( _timerRunning && ev->timerId() == _timerId ) {
		_broker->currentTask()->incrementTime( 1 );
		updateCurrentItem();

		emit timerTick();
	}
}

void Karm::updateCurrentItem()
{
	formatTime( _timeStr, _broker->currentTask()->time() );

	_timeList->changeItem( _timeStr, _broker->current() );
	_nameList->changeItem( _broker->currentTask()->name(), 
				_broker->current() );
}

void Karm::newTask()
{
	if( _addDlg == 0 ) {
		// popup a dialog asking for info
		_addDlg = new KarmAddDlg;
               _addDlg->setCaption(KarmName + i18n( ": Add New Task" ) );

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
		_broker->addTask( _addDlg->taskName(),
				  _addDlg->taskTime() );
		fillListBoxes();

		moveTo( _broker->count() - 1);
	}

	// dispose of the dialog
	delete _addDlg;
	_addDlg = 0;
}

void Karm::editTask()
{
	if( _broker->currentTask() == 0 ) {
		return;
	}

	if( _editDlg == 0 ) {
		_editDlg = new KarmAddDlg;

               _editDlg->setCaption( KarmName + i18n( ": Edit Task Entry" ) );
		_editDlg->setTask( _broker->currentTask()->name(),
				_broker->currentTask()->time() );

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
		int currentItem = _broker->current();

		_broker->currentTask()->setName( _editDlg->taskName() );
		_broker->currentTask()->setTime( _editDlg->taskTime() );

		fillListBoxes();

		_broker->moveTo( currentItem );	
	}

	delete _editDlg;
	_editDlg = 0;
}

void Karm::deleteTask()
{
	if( _broker->count() == 0 )
		return;

       int response = KMsgBox::yesNo(0, KarmName + i18n( ": Deleting Task" ) , 
               i18n( "Are you sure you want to delete this task?" ), 
		KMsgBox::QUESTION,
               i18n( "&Yes" ), i18n( "&Cancel" ) );

	if( response == 1 ) {
		// save the current position
		int position = _broker->current() - 1;

		// delete the item
		_broker->deleteCurrentTask();
		fillListBoxes();

		// move to our new current item
		if( position >= 0 && _broker->count() > position )
			moveTo( position );			

		if( _broker->count() == 0 )
			stopClock();

	}
}
