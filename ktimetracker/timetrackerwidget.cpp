/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

#include "timetrackerwidget.h"

#include <QDBusConnection>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>

#include <KApplication>
#include <KDebug>
#include <KFileDialog>
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <KTabWidget>
#include <KTemporaryFile>
#include <KTreeWidgetSearchLine>
#include <KIO/Job>

#include "karmerrors.h"
#include "mainadaptor.h"
#include "task.h"
#include "taskview.h"
#include "version.h"

//@cond PRIVATE
class TimetrackerWidget::Private {
  public:
    Private() :
      mLastView( 0 ) {}

    QWidget *mSearchLine;
    KTabWidget *mTabWidget;
    KTreeWidgetSearchLine *mSearchWidget;
    TaskView *mLastView;
    QVector<TaskView*> mIsNewVector;
};
//@endcond

TimetrackerWidget::TimetrackerWidget( QWidget *parent ) : QWidget( parent ),
  d( new TimetrackerWidget::Private() )
{
  new MainAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/KTimeTracker", this );

  QLayout *layout = new QVBoxLayout;
  layout->setMargin( 0 );
  layout->setSpacing( 0 );

  QLayout *innerLayout = new QHBoxLayout;
  d->mSearchLine = new QWidget( this );
  innerLayout->setMargin( KDialog::marginHint() );
  innerLayout->setSpacing( KDialog::spacingHint() );
  d->mSearchWidget = new KTreeWidgetSearchLine( d->mSearchLine );
  d->mSearchWidget->setClickMessage( i18n( "Search or add task" ) );
  innerLayout->addWidget( d->mSearchWidget );
  d->mSearchLine->setLayout( innerLayout );

  d->mTabWidget = new KTabWidget( this );
  layout->addWidget( d->mSearchLine );
  layout->addWidget( d->mTabWidget );
  setLayout( layout );

  d->mTabWidget->setFocus( Qt::OtherFocusReason );

  connect( d->mSearchWidget, SIGNAL( returnPressed( const QString& ) ),
           this, SLOT( slotAddTask( const QString& ) ) );
  connect( d->mTabWidget, SIGNAL( currentChanged( int ) ), 
           this, SIGNAL( currentTaskViewChanged() ) );
  connect( d->mTabWidget, SIGNAL( currentChanged( int ) ),
           this, SLOT( slotCurrentChanged() ) );
}

TimetrackerWidget::~TimetrackerWidget()
{
  delete d;
}

void TimetrackerWidget::addTaskView( const QString &fileName )
{
  bool isNew = fileName.isEmpty();
  QString lFileName = fileName;

  if ( isNew ) {
    KTemporaryFile tempFile;
    tempFile.setAutoRemove( false );
    if ( tempFile.open() ) {
      lFileName = tempFile.fileName();
      tempFile.close();
    } else {
      KMessageBox::error( this, i18n( "Cannot create new file." ) );
      return;
    }
  }

  TaskView *taskView = new TaskView( this );
  connect( taskView, SIGNAL( contextMenuRequested( const QPoint& ) ), 
           this, SIGNAL( contextMenuRequested( const QPoint& ) ) );
  connect( taskView, SIGNAL( tasksChanged( const QList< Task* >& ) ),
           this, SLOT( updateTabs() ) );

  d->mTabWidget->addTab( taskView, 
          isNew ? KIcon( "document-save" ) : KIcon( "karm" ), 
          isNew ? i18n( "Untitled" ) : QFileInfo( lFileName ).fileName() );
  d->mTabWidget->setCurrentWidget( taskView );
  taskView->load( lFileName );
  d->mSearchWidget->addTreeWidget( taskView );

  if ( isNew ) {
    d->mIsNewVector.append( taskView );
  } else {
    // FIXME does not work for the startup page
    d->mTabWidget->setTabToolTip( d->mTabWidget->currentIndex(), lFileName );
  }

  // When adding the first tab currentChanged is not emitted, so...
  if ( !d->mLastView ) {
    emit currentTaskViewChanged();
    slotCurrentChanged();
  }
}

bool TimetrackerWidget::saveCurrentTaskView()
{
  QString fileName = KFileDialog::getSaveFileName( QString(), QString(), this );
  if ( !fileName.isEmpty() ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );
    taskView->stopAllTimers();
    taskView->save();
    taskView->closeStorage();

    QString currentFilename = taskView->storage()->icalfile();
    KIO::file_move( currentFilename, fileName, -1, true, false, false );
    d->mIsNewVector.remove( d->mIsNewVector.indexOf( taskView ) );

    taskView->load( fileName );
    KIO::file_delete( currentFilename, false );

    d->mTabWidget->setTabIcon( d->mTabWidget->currentIndex(), KIcon( "karm" ) );
    d->mTabWidget->setTabText( d->mTabWidget->currentIndex(), QFileInfo( fileName ).fileName() );
    d->mTabWidget->setTabToolTip( d->mTabWidget->currentIndex(), fileName );

    return true;
  }

  return false;
}

TaskView* TimetrackerWidget::currentTaskView()
{
  return qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );
}

Task* TimetrackerWidget::currentTask()
{
  TaskView *taskView = 0;
  if ( ( taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() ) ) ) {
    return taskView->currentItem();
  } else {
    return 0;
  }
}

void TimetrackerWidget::newFile()
{
  addTaskView();
}

void TimetrackerWidget::openFile( const QString &fileName )
{
  addTaskView( fileName );
}

bool TimetrackerWidget::closeFile()
{
  TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

  // is it an unsaved file?
  if ( d->mIsNewVector.contains( taskView ) ) {
    QString message = i18n( "This document has not been saved yet. Do you want to save it?" );
    QString caption = i18n( "Untitled" );

    int result = KMessageBox::questionYesNoCancel( this, message, caption );

    if ( result == KMessageBox::Cancel ) {
      return false;
    }

    if ( result == KMessageBox::Yes ) {
      if ( !saveCurrentTaskView() ) {
        return false;
      }
    } else { // result == No
      d->mIsNewVector.remove( d->mIsNewVector.indexOf( taskView ) );
    }
  }

  taskView->stopAllTimers();
  taskView->save();
  taskView->closeStorage();

  d->mTabWidget->removeTab( d->mTabWidget->currentIndex() );
  d->mSearchWidget->removeTreeWidget( taskView );

  /* emit signals and call slots since currentChanged is not emitted if
   * we close the last tab
   */
  if ( d->mTabWidget->count() == 0 ) {
    emit currentTaskViewChanged();
    slotCurrentChanged();
  }

  delete taskView; // removeTab does not delete its widget.
  return true;
}

QString TimetrackerWidget::saveFile() 
{
  TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

  // is it an unsaved file?
  if ( d->mIsNewVector.contains( taskView ) ) {
    saveCurrentTaskView();
  }

  return taskView->save();
}

void TimetrackerWidget::reconfigureFiles()
{
  TaskView *taskView;
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    taskView->reconfigure();
  }
}

void TimetrackerWidget::showSearchBar( bool visible )
{
  d->mSearchLine->setVisible( visible );
}

bool TimetrackerWidget::closeAllFiles()
{
  while ( d->mTabWidget->count() > 0 ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( 0 ) );

    d->mTabWidget->setCurrentWidget( taskView );
    if ( !( closeFile() ) )
      return false;
  }

  return true;
}

void TimetrackerWidget::slotCurrentChanged()
{
  kDebug() << "entering KTimetrackerWidget::slotCurrentChanged";

  if ( d->mLastView ) {
    disconnect( d->mLastView, SIGNAL( totalTimesChanged( long, long ) ) );
    disconnect( d->mLastView, SIGNAL( itemSelectionChanged() ) );
    disconnect( d->mLastView, SIGNAL( updateButtons() ) );
    disconnect( d->mLastView, SIGNAL( setStatusBarText( QString ) ) );
    disconnect( d->mLastView, SIGNAL( timersActive() ) );
    disconnect( d->mLastView, SIGNAL( timersInactive() ) );
    disconnect( d->mLastView, SIGNAL( tasksChanged( const QList< Task* >& ) ),
                this, SIGNAL( tasksChanged( const QList< Task* > & ) ) );
  }

  d->mLastView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

  if ( d->mLastView ) {
    connect( d->mLastView, SIGNAL( totalTimesChanged( long, long ) ),
            this, SIGNAL( totalTimesChanged( long, long ) ) ); 
    connect( d->mLastView, SIGNAL( itemSelectionChanged() ),
            this, SIGNAL( currentTaskChanged() ) );
    connect( d->mLastView, SIGNAL( updateButtons() ),
            this, SIGNAL( updateButtons() ) );
    connect( d->mLastView, SIGNAL( setStatusBarText( QString ) ), // FIXME signature
            this, SIGNAL( statusBarTextChangeRequested( const QString & ) ) );
    connect( d->mLastView, SIGNAL( timersActive() ),
            this, SIGNAL( timersActive() ) );
    connect( d->mLastView, SIGNAL( timersInactive() ),
            this, SIGNAL( timersInactive() ) );
    connect( d->mLastView, SIGNAL( tasksChanged( QList< Task* > ) ), // FIXME signature
            this, SIGNAL( tasksChanged( const QList< Task* > &) ) );
  }

  d->mSearchWidget->setEnabled( d->mLastView );
}

void TimetrackerWidget::updateTabs()
{
  TaskView *taskView;
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( taskView->activeTasks().count() == 0 ) {
      d->mTabWidget->setTabTextColor( i, palette().color( QPalette::Foreground ) );
    } else {
      d->mTabWidget->setTabTextColor( i, Qt::darkGreen );
    }
  }
}

void TimetrackerWidget::slotAddTask( const QString &taskName )
{
  TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

  taskView->addTask( taskName, 0, 0, DesktopList(), 0 );

  d->mSearchWidget->clear();
  d->mTabWidget->setFocus( Qt::OtherFocusReason );
}

//BEGIN wrapper slots
void TimetrackerWidget::startCurrentTimer()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->startCurrentTimer();
  }
}

void TimetrackerWidget::stopCurrentTimer()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->stopCurrentTimer();
  }
}

void TimetrackerWidget::stopAllTimers( const QDateTime &when )
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->stopAllTimers( when );
  }
}

void TimetrackerWidget::newTask()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->newTask();
  }
}

void TimetrackerWidget::newSubTask()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->newSubTask();
  }
}

void TimetrackerWidget::editTask()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->editTask();
  }
}

void TimetrackerWidget::deleteTask( bool markingascomplete )
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->deleteTask( markingascomplete );
  }
}

void TimetrackerWidget::markTaskAsComplete()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->markTaskAsComplete();
  }
}

void TimetrackerWidget::markTaskAsIncomplete()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->markTaskAsIncomplete();
  }
}

void TimetrackerWidget::exportcsvFile()
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->exportcsvFile();
  }
}

void TimetrackerWidget::importPlanner( const QString &fileName )
{
  if ( d->mTabWidget->currentWidget() ) {
    qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->importPlanner( fileName );
  }
}
//END

//BEGIN dbus slots
QString TimetrackerWidget::version() const
{
  return KTIMETRACKER_VERSION;
}

QStringList TimetrackerWidget::taskIdsFromName( const QString &taskName ) const
{
  QStringList result;

  for ( int i = 0; i < d->mTabWidget->count(); ++i) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      Task *task = static_cast< Task* >( *it );
      if ( task && task->name() == taskName ) {
        result << task->uid();
      }
      ++it;
    }
  }

  return result;
}

void TimetrackerWidget::addTask( const QString &taskName )
{
  TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

  if ( taskView ) {
    taskView->addTask( taskName, 0, 0, DesktopList(), 0 );
  }
}

void TimetrackerWidget::setPercentComplete( const QString &taskId, int percent )
{
  for ( int i = 0; i < d->mTabWidget->count(); ++i) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      Task *task = static_cast< Task* >( *it );
      if ( task && task->uid() == taskId ) {
        task->setPercentComplete( percent, taskView->storage() );
      }
      ++it;
    }
  }
}

int TimetrackerWidget::bookTime( const QString &taskId, QString dateTime, int minutes )
{
  QDate startDate;
  QTime startTime;
  QDateTime startDateTime;
  Task *task = 0, *t = 0;

  if ( minutes <= 0 ) return KARM_ERR_INVALID_DURATION;

  // Find task
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      t = static_cast< Task* >( *it );
      if ( t && t->uid() == taskId ) {
        task = t;
        break;
      }
      ++it;
    }

    if ( task ) break;
  }

  if ( !task ) return KARM_ERR_UID_NOT_FOUND;

  // Parse datetime
  startDate = QDate::fromString( dateTime, Qt::ISODate );

  if ( dateTime.length() > 10 ) { // "YYYY-MM-DD".length() = 10
    startTime = QTime::fromString( dateTime, Qt::ISODate );
  } else startTime = QTime( 12, 0 );

  if ( startDate.isValid() && startTime.isValid() ) {
    startDateTime = QDateTime( startDate, startTime );
  } else return KARM_ERR_INVALID_DATE;

  // Update task totals (session and total) and save to disk
  task->changeTotalTimes( task->sessionTime() + minutes, 
                          task->totalTime() + minutes );
  if ( !( task->taskView()->storage()->bookTime( task, 
                                                 startDateTime, 
                                                 minutes * 60 ) ) )
    return KARM_ERR_GENERIC_SAVE_FAILED;

  return 0;
}

QString TimetrackerWidget::error( int errorCode ) const
{
  switch ( errorCode ) {
    case KARM_ERR_GENERIC_SAVE_FAILED:
      return i18n( "Save failed, most likely because the file could not be locked." );
    case KARM_ERR_COULD_NOT_MODIFY_RESOURCE:
      return i18n( "Could not modify calendar resource." );
    case KARM_ERR_MEMORY_EXHAUSTED:
      return i18n( "Out of memory--could not create object." );
    case KARM_ERR_UID_NOT_FOUND:
      return i18n( "UID not found." );
    case KARM_ERR_INVALID_DATE:
      return i18n( "Invalidate date--format is YYYY-MM-DD." );
    case KARM_ERR_INVALID_TIME:
      return i18n( "Invalid time--format is YYYY-MM-DDTHH:MM:SS." );
    case KARM_ERR_INVALID_DURATION:
      return i18n( "Invalid task duration--must be greater than zero." );
    default:
      return i18n( "Invalid error number: %1", errorCode );
  }
}

int TimetrackerWidget::totalMinutesForTaskId( const QString &taskId ) const
{
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      Task *task = static_cast< Task* >( *it );

      if ( task && task->uid() == taskId ) {
        return task->totalTime();
      }

      ++it;
    }
  }

  return -1;
}

void TimetrackerWidget::startTimerFor( const QString &taskId )
{
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      Task *task = static_cast< Task* >( *it );

      if ( task && task->uid() == taskId ) {
        taskView->startTimerFor( task );
        return;
      }

      ++it;
    }
  }
}

void TimetrackerWidget::stopTimerFor( const QString &taskId )
{
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      Task *task = static_cast< Task* >( *it );

      if ( task && task->uid() == taskId ) {
        taskView->stopTimerFor( task );
        return;
      }

      ++it;
    }
  }
}

void TimetrackerWidget::stopAllTimersDBUS()
{
  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    taskView->stopAllTimers();
  }
}

QStringList TimetrackerWidget::tasks() const
{
  QStringList result;

  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
      result << static_cast< Task* >( *it )->name();
      ++it;
    }
  }

  return result;
}

QStringList TimetrackerWidget::activeTasks() const
{
  QStringList result;

  for ( int i = 0; i < d->mTabWidget->count(); ++i ) {
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

    if ( !taskView ) continue;

    for ( int j = 0; j < taskView->count(); ++j ) {
      if ( taskView->itemAt( j )->isRunning() ) {
        result << taskView->itemAt( j )->name();
      }
    }
  }

  return result;
}

void TimetrackerWidget::quit()
{
  if ( closeAllFiles() ) {
    kapp->quit();
  }
}
//END

#include "timetrackerwidget.moc"
