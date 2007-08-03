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

#include <QFileInfo>
#include <QVector>

#include <KDebug>
#include <KFileDialog>
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <KTemporaryFile>
#include <KIO/Job>

#include "task.h"
#include "taskview.h"

//@cond PRIVATE
class TimetrackerWidget::Private {
  public:
    Private() : 
      mLastView( 0 ) {}

    TaskView *mLastView;
    QVector<TaskView*> mIsNewVector;
};
//@endcond

TimetrackerWidget::TimetrackerWidget( QWidget *parent ) : KTabWidget( parent ),
  d( new TimetrackerWidget::Private() )
{
  connect( this, SIGNAL( currentChanged( int ) ), 
           this, SIGNAL( currentTaskViewChanged() ) );
  connect( this, SIGNAL( currentChanged( int ) ),
           this, SLOT( slotCurrentChanged() ) );
}

TimetrackerWidget::~TimetrackerWidget()
{
  // FIXME this has to be moved to something like hasUnsavedFiles()
  // b/c the dialogs are called without seeing the mainframe.
  while ( count() > 0 ) {
    TaskView *taskView = qobject_cast< TaskView* >( widget( 0 ) );

    setCurrentWidget( taskView );
    closeFile( false );
  }
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
  taskView->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( taskView, SIGNAL( customContextMenuRequested( const QPoint& ) ), 
           this, SIGNAL( contextMenuRequested( const QPoint& ) ) );
  connect( taskView, SIGNAL( tasksChanged( const QList< Task* >& ) ),
           this, SLOT( updateTabs() ) );

  addTab( taskView, 
          isNew ? KIcon( "document-save" ) : KIcon( "karm" ), 
          isNew ? i18n( "Untitled" ) : QFileInfo( lFileName ).fileName() );
  setCurrentWidget( taskView );
  taskView->load( lFileName );

  if ( isNew ) {
    d->mIsNewVector.append( taskView );
  } else {
    // FIXME does not work for the startup page
    setTabToolTip( currentIndex(), lFileName );
  }

  // When adding the first tab currentChanged is not emitted, so...
  if ( !d->mLastView ) {
    emit currentChanged( 0 );
  }
}

bool TimetrackerWidget::saveCurrentTaskView()
{
  QString fileName = KFileDialog::getSaveFileName( QString(), QString(), this );
  if ( !fileName.isEmpty() ) {
    TaskView *taskView = qobject_cast< TaskView* >( currentWidget() );
    taskView->stopAllTimers();
    taskView->save();
    taskView->closeStorage();

    QString currentFilename = taskView->storage()->icalfile();
    KIO::file_move( currentFilename, fileName, -1, true, false, false );
    d->mIsNewVector.remove( d->mIsNewVector.indexOf( taskView ) );

    taskView->load( fileName );
    KIO::file_delete( currentFilename, false );

    setTabIcon( currentIndex(), KIcon( "karm" ) );
    setTabText( currentIndex(), QFileInfo( fileName ).fileName() );
    setTabToolTip( currentIndex(), fileName );

    return true;
  }

  return false;
}

TaskView* TimetrackerWidget::currentTaskView()
{
  return qobject_cast< TaskView* >( currentWidget() );
}

Task* TimetrackerWidget::currentTask()
{
  TaskView *taskView = 0;
  if ( ( taskView = qobject_cast< TaskView* >( currentWidget() ) ) ) {
    return taskView->current_item();
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

void TimetrackerWidget::closeFile( bool canCancel )
{
  TaskView *taskView = qobject_cast< TaskView* >( currentWidget() );

  // is it an unsaved file?
  if ( d->mIsNewVector.contains( taskView ) ) {
    QString message = i18n( "This document has not been saved yet. Do you want to save it?" );
    QString caption = i18n( "Untitled" );

    int result;
    if ( canCancel ) {
      result = KMessageBox::questionYesNoCancel( this, message, caption );
    } else {
      result = KMessageBox::questionYesNo( this, message, caption );
    }

    if ( result == KMessageBox::Cancel ) {
      return;
    }

    if ( result == KMessageBox::Yes ) {
      if ( !saveCurrentTaskView() ) {
        return;
      }
    } else { // result == No
      d->mIsNewVector.remove( d->mIsNewVector.indexOf( taskView ) );
    }

    taskView->stopAllTimers();
    taskView->save();
    taskView->closeStorage();
  }

  removeTab( currentIndex() );

  /* emit currentChanged signal since currentChanged is not emitted if
   * we close the last tab
   */
  if ( count() == 0 ) {
    emit currentChanged( -1 );
  }

  delete taskView; // removeTab does not delete its widget.
}

QString TimetrackerWidget::saveFile() 
{
  TaskView *taskView = qobject_cast< TaskView* >( currentWidget() );

  // is it an unsaved file?
  if ( d->mIsNewVector.contains( taskView ) ) {
    saveCurrentTaskView();
  }

  return taskView->save();
}

void TimetrackerWidget::reconfigureFiles()
{
  TaskView *taskView;
  for ( int i = 0; i < count(); ++i ) {
    taskView = qobject_cast< TaskView* >( widget( i ) );

    taskView->reconfigure();
  }
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

  d->mLastView = qobject_cast< TaskView* >( currentWidget() );

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
}

void TimetrackerWidget::updateTabs()
{
  TaskView *taskView;
  for ( int i = 0; i < count(); ++i ) {
    taskView = qobject_cast< TaskView* >( widget( i ) );

    if ( taskView->activeTasks().count() == 0 ) {
      setTabTextColor( i, palette().color( QPalette::Foreground ) );
    } else {
      setTabTextColor( i, Qt::darkGreen );
    }
  }
}

//BEGIN Wrapper Slots
void TimetrackerWidget::startCurrentTimer()
{
  qobject_cast< TaskView* >( currentWidget() )->startCurrentTimer();
}

void TimetrackerWidget::stopCurrentTimer()
{
  qobject_cast< TaskView* >( currentWidget() )->stopCurrentTimer();
}

void TimetrackerWidget::stopAllTimers( const QDateTime &when )
{
  qobject_cast< TaskView* >( currentWidget() )->stopAllTimers( when );
}

void TimetrackerWidget::newTask()
{
  qobject_cast< TaskView* >( currentWidget() )->newTask();
}

void TimetrackerWidget::newSubTask()
{
  qobject_cast< TaskView* >( currentWidget() )->newSubTask();
}

void TimetrackerWidget::editTask()
{
  qobject_cast< TaskView* >( currentWidget() )->editTask();
}

void TimetrackerWidget::deleteTask( bool markingascomplete )
{
  qobject_cast< TaskView* >( currentWidget() )->deleteTask( markingascomplete );
}

void TimetrackerWidget::markTaskAsComplete()
{
  qobject_cast< TaskView* >( currentWidget() )->markTaskAsComplete();
}

void TimetrackerWidget::markTaskAsIncomplete()
{
  qobject_cast< TaskView* >( currentWidget() )->markTaskAsIncomplete();
}

void TimetrackerWidget::exportcsvFile()
{
  qobject_cast< TaskView* >( currentWidget() )->exportcsvFile();
}

void TimetrackerWidget::importPlanner( const QString &fileName )
{
  qobject_cast< TaskView* >( currentWidget() )->importPlanner( fileName );
}
//END

#include "timetrackerwidget.moc"
