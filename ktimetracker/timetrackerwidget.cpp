/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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
#include "ktimetrackerconfigdialog.h"

#include <QDBusConnection>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMap>
#include <QVBoxLayout>
#include <QVector>

#include <KApplication>
#include <KAction>
#include <KActionCollection>
#include <KConfig>
#include <KConfigDialog>
#include <KDebug>
#include <KFileDialog>
#include <KGlobal>
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KStandardAction>
#include <KTabWidget>
#include <KTemporaryFile>
#include <KTreeWidgetSearchLine>
#include <KUrl>
#include <KIO/Job>

#include "historydialog.h"
#include "idletimedetector.h"
#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "mainadaptor.h"
#include "reportcriteria.h"
#include "task.h"
#include "taskview.h"
#include "version.h"

//@cond PRIVATE
class TimetrackerWidget::Private
{
  public:
    Private() :
      mLastView( 0 ), mRecentFilesAction( 0 ) {}

    QWidget *mSearchLine;
    KTabWidget *mTabWidget;
    KTreeWidgetSearchLine *mSearchWidget;
    TaskView *mLastView;
    QVector<TaskView*> mIsNewVector;
    QMap<QString, KAction*> mActions;
    KRecentFilesAction *mRecentFilesAction;

    struct ActionData
    {
        QString iconName;
        const char* caption;
        const char* slot;
        QString name;
        const char* toolTip;
        const char* whatsThis;
    };
};
//@endcond

TimetrackerWidget::TimetrackerWidget( QWidget *parent ) : QWidget( parent ),
  d( new TimetrackerWidget::Private() )
{
    kDebug(5970) << "Entering function";
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
    d->mSearchWidget->setWhatsThis( i18n( "This is a combined field. As long as you do not type ENTER, it acts as a filter. Then, only tasks that match your input are shown. As soon as you type ENTER, your input is used as name to create a new task." ) );
    d->mSearchWidget->installEventFilter( this );
    innerLayout->addWidget( d->mSearchWidget );
    d->mSearchLine->setLayout( innerLayout );

    d->mTabWidget = new KTabWidget( this );
    layout->addWidget( d->mSearchLine );
    layout->addWidget( d->mTabWidget );
    setLayout( layout );

    d->mTabWidget->setFocus( Qt::OtherFocusReason );

    connect( d->mTabWidget, SIGNAL( currentChanged( int ) ),
           this, SIGNAL( currentTaskViewChanged() ) );
    connect( d->mTabWidget, SIGNAL( currentChanged( int ) ),
           this, SLOT( slotCurrentChanged() ) );
    connect( d->mTabWidget, SIGNAL( mouseDoubleClick() ),
           this, SLOT( newFile() ) );

    showSearchBar( !KTimeTrackerSettings::configPDA() );
    showTabBar( false );
}

TimetrackerWidget::~TimetrackerWidget()
{
    if ( d->mRecentFilesAction )
    {
        d->mRecentFilesAction->saveEntries( KGlobal::config()->group( "Recent Files" ) );
    }
    delete d;
}

bool TimetrackerWidget::allEventsHaveEndTiMe()
{
    bool result=true;
    TaskView *taskView;
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView->allEventsHaveEndTiMe() ) result=false;
    }
    return result;
}

void TimetrackerWidget::saveProperties( KConfigGroup &cfg )
{
    cfg.writeEntry( "WindowShown", isVisible());
}

void TimetrackerWidget::readProperties( const KConfigGroup &cfg )
{
    if( cfg.readEntry( "WindowShown", true ))
        show();
}

int TimetrackerWidget::focusSearchBar()
{
    kDebug(5970) << "Entering function";
    if ( d->mSearchWidget->isVisible() ) d->mSearchWidget->setFocus();
    return 0;
}

void TimetrackerWidget::addTaskView( const QString &fileName )
{
    kDebug(5970) << "Entering function (fileName=" << fileName << ")";
    bool isNew = fileName.isEmpty();
    QString lFileName = fileName;

    if ( isNew )
    {
        KTemporaryFile tempFile;
        tempFile.setAutoRemove( false );
        if ( tempFile.open() )
        {
            lFileName = tempFile.fileName();
            tempFile.close();
        }
        else
        {
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
          isNew ? KIcon( "document-save" ) : KIcon( "ktimetracker" ),
          isNew ? i18n( "Untitled" ) : QFileInfo( lFileName ).fileName() );
    d->mTabWidget->setCurrentWidget( taskView );
    emit setCaption( fileName );
    taskView->load( lFileName );
    d->mSearchWidget->addTreeWidget( taskView );

    if ( isNew )
    {
        d->mIsNewVector.append( taskView );
    } else
    {
        // FIXME does not work for the startup page
        d->mTabWidget->setTabToolTip( d->mTabWidget->currentIndex(), lFileName );
    }

    // When adding the first tab currentChanged is not emitted, so...
    if ( !d->mLastView )
    {
        emit currentTaskViewChanged();
        slotCurrentChanged();
    }

    if ( d->mTabWidget->count() > 1 )
    {
        showTabBar( true );
    }
}

bool TimetrackerWidget::saveCurrentTaskView()
{
    QString fileName = KFileDialog::getSaveFileName( QString(), QString(), this );
    if ( !fileName.isEmpty() )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );
        taskView->stopAllTimers();
        taskView->save();
        taskView->closeStorage();

        QString currentFilename = taskView->storage()->icalfile();
        KIO::file_move( currentFilename, fileName, -1, KIO::Overwrite | KIO::HideProgressInfo );
        d->mIsNewVector.remove( d->mIsNewVector.indexOf( taskView ) );

        taskView->load( fileName );
        KIO::file_delete( currentFilename, KIO::HideProgressInfo );

        d->mTabWidget->setTabIcon( d->mTabWidget->currentIndex(), KIcon( "ktimetracker" ) );
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
    if ( ( taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() ) ) )
    {
        return taskView->currentItem();
    }
    else
    {
        return 0;
    }
}

void TimetrackerWidget::setupActions( KActionCollection *actionCollection )
{
    d->mActions.insert( "file_new",
        KStandardAction::openNew( this, SLOT( newFile() ), actionCollection ) );
    d->mActions[ "file_new" ]->setIcon( KIcon( "tab-new" ) );

    d->mActions.insert ("file_open",
        KStandardAction::open( this, SLOT( openFile() ), actionCollection ) );

    d->mRecentFilesAction = KStandardAction::openRecent(
        this, SLOT( openFile( const KUrl & ) ), this );
    actionCollection->addAction( d->mRecentFilesAction->objectName(),
                               d->mRecentFilesAction );
    d->mRecentFilesAction->loadEntries( KGlobal::config()->group( "Recent Files" ) );

    d->mActions.insert ("file_save",
        KStandardAction::save( this, SLOT( saveFile() ), actionCollection ) );
    d->mActions.insert ("file_close",
        KStandardAction::close( this, SLOT( closeFile() ), actionCollection ) );
    d->mActions.insert ("file_quit",
        KStandardAction::quit( this, SLOT( quit() ), actionCollection ) );
    d->mActions.insert ("configure_ktimetracker",
        KStandardAction::preferences( this, SLOT( showSettingsDialog() ),
                                  actionCollection ) );

    Private::ActionData actions[] =
    {
        { QString(), I18N_NOOP("Start &New Session"), SLOT( startNewSession() ),
            "start_new_session", I18N_NOOP("Starts a new session"), I18N_NOOP("This will reset the "
            "session time to 0 for all tasks, to start a new session, without "
            "affecting the totals.")
        },
        { "view-history", I18N_NOOP("Edit History..."), SLOT( editHistory() ), "edit_history",
            I18N_NOOP("Edits history of all tasks of the current document"), I18N_NOOP("A window will "
            "be opened where you can change start and stop times of tasks or add a "
            "comment to them.")
        },
        { QString(), I18N_NOOP("&Reset All Times"), SLOT( resetAllTimes() ),
            "reset_all_times", I18N_NOOP("Resets all times"), I18N_NOOP("This will reset the session "
            "and total time to 0 for all tasks, to restart from scratch.")
        },
        { "media-playback-start", I18N_NOOP("&Start"), SLOT( startCurrentTimer() ), "start",
            I18N_NOOP("Starts timing for selected task"), I18N_NOOP("This will start timing for the "
            "selected task.\nIt is even possible to time several tasks "
            "simultanously.\n\nYou may also start timing of tasks by double clicking "
            "the left mouse button on a given task. This will, however, stop timing "
            "of other tasks.")
        },
        { "media-playback-stop", I18N_NOOP("S&top"), SLOT( stopCurrentTimer() ), "stop",
            I18N_NOOP("Stops timing of the selected task"), I18N_NOOP("Stops timing of the selected task")
        },
        { QString(), I18N_NOOP("Focus on Searchbar"), SLOT( focusSearchBar() ), "focusSearchBar",
            I18N_NOOP("Sets the focus on the searchbar"), I18N_NOOP("Sets the focus on the searchbar")
        },
        { QString(), I18N_NOOP("Stop &All Timers"), SLOT( stopAllTimers() ), "stopAll",
            I18N_NOOP("Stops all of the active timers"), I18N_NOOP("Stops all of the active timers")
        },
        { QString(), I18N_NOOP("Track Active Applications"), SLOT( focusTracking() ),
            "focustracking", I18N_NOOP("Auto-creates and updates tasks when the focus of the "
            "current window has changed"), I18N_NOOP("If the focus of a window changes for the "
            "first time when this action is enabled, a new task will be created "
            "with the title of the window as its name and will be started. If there "
            "already exists such an task it will be started.")
        },
        // in the following element,
        // document-new is the icon name, e.g. for the toolbar, new_task is how the action is
        // called in the ktimetrackerui.rc file, rest should be obvious
        { "document-new", I18N_NOOP("&New Task..."), SLOT( newTask() ), "new_task", I18N_NOOP("Creates "
            "new top level task"), I18N_NOOP("This will create a new top level task.")
        },
        { "subtask-new-ktimetracker", I18N_NOOP("New &Subtask..."), SLOT( newSubTask() ),
            "new_sub_task", I18N_NOOP("Creates a new subtask to the current selected task"),
            I18N_NOOP("This will create a new subtask to the current selected task.")
        },
        { "edit-delete", I18N_NOOP("&Delete"), SLOT( deleteTask() ), "delete_task", I18N_NOOP("Deletes "
            "selected task"), I18N_NOOP("This will delete the selected task(s) and all "
            "subtasks.")
        },
        { "document-properties", I18N_NOOP("&Edit..."), SLOT( editTask() ), "edit_task",
            I18N_NOOP("Edits name or times for selected task"), I18N_NOOP("This will bring up a dialog "
            "box where you may edit the parameters for the selected task.")
        },
        { QString(), I18N_NOOP("&Mark as Complete"), SLOT( markTaskAsComplete() ),
            "mark_as_complete", "", ""
        },
        { QString(), I18N_NOOP("&Mark as Incomplete"), SLOT( markTaskAsIncomplete() ),
            "mark_as_incomplete", "", ""
        },
        { QString(), I18N_NOOP("&Export Times..."), SLOT( exportcsvFile() ), "export_times",
            "", ""
        },
        { QString(), I18N_NOOP("Export &History..."), SLOT( exportcsvHistory() ),
            "export_history", "", ""
        },
        { QString(), I18N_NOOP("Import Tasks From &Planner..."), SLOT( importPlanner() ),
            "import_planner", "", ""
        },
        { QString(), I18N_NOOP("Show Searchbar"), SLOT( slotSearchBar() ), "searchbar",
            "", ""
        }
    };

    for ( unsigned int i = 0; i < ( sizeof( actions ) / sizeof( Private::ActionData ) ); ++i )
    {
        Private::ActionData actionData = actions[i];
        KAction *action;
        if ( actionData.iconName.isEmpty() )
        {
            action = new KAction( i18n( actionData.caption ), this );
        } else
        {
            action = new KAction( KIcon( actionData.iconName ),
                            i18n( actionData.caption ), this );
        }

        actionCollection->addAction( actionData.name, action );
        connect( action, SIGNAL( triggered( bool ) ), actionData.slot );
        action->setToolTip( i18n( actionData.toolTip ) );
        action->setWhatsThis( i18n( actionData.whatsThis ) );

        d->mActions.insert( actionData.name, action );
    }

    // custom shortcuts
    d->mActions[ "stopAll" ]->setShortcut( QKeySequence( Qt::Key_Escape ) );
    d->mActions[ "new_task" ]->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_T ) );
    d->mActions[ "focusSearchBar" ]->setShortcut( QKeySequence( Qt::Key_S ) );
    d->mActions[ "new_sub_task" ]->setShortcut( QKeySequence( Qt::CTRL + Qt::ALT + Qt::Key_N ) );
    d->mActions[ "delete_task" ]->setShortcut( QKeySequence( Qt::Key_Delete ) );
    d->mActions[ "edit_task" ]->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_E ) );
    d->mActions[ "mark_as_complete" ]->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_M ) );
    d->mActions[ "mark_as_incomplete" ]->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_M ) );

    d->mActions[ "mark_as_complete" ]->setIcon( UserIcon( "task-complete.xpm" ) );
    d->mActions[ "mark_as_incomplete" ]->setIcon( UserIcon( "task-incomplete.xpm" ) );

    d->mActions[ "focustracking" ]->setCheckable( true );
    d->mActions[ "searchbar" ]->setCheckable( true );

    d->mActions[ "searchbar" ]->setChecked( KTimeTrackerSettings::self()->showSearchBar() );

    connect( this, SIGNAL( currentTaskChanged() ),
             this, SLOT( slotUpdateButtons() ) );
    connect( this, SIGNAL( currentTaskViewChanged() ),
             this, SLOT( slotUpdateButtons() ) );
    connect( this, SIGNAL( updateButtons() ),
             this, SLOT( slotUpdateButtons() ) );
}

KAction* TimetrackerWidget::action( const QString &name ) const
{
    return d->mActions.value( name );
}

void TimetrackerWidget::newFile()
{
    addTaskView();
}

void TimetrackerWidget::openFile( const QString &fileName )
{
    kDebug(5970) << "Entering function, fileName is " << fileName;
    QString newFileName = fileName;
    if ( newFileName.isEmpty() )
    {
        newFileName = KFileDialog::getOpenFileName( QString(), QString(),
                                                        this );
        if ( newFileName.isEmpty() )
        {
            return;
        }
    }

    if ( d->mRecentFilesAction )
    {
        d->mRecentFilesAction->addUrl( newFileName );
    }
    addTaskView( newFileName );
}

void TimetrackerWidget::openFile( const KUrl &fileName )
{
    openFile( fileName.toLocalFile() );
}

bool TimetrackerWidget::closeFile()
{
    kDebug(5970) << "Entering TimetrackerWidget::closeFile";
    TaskView *taskView = (TaskView*)( d->mTabWidget->currentWidget() );

    // is it an unsaved file?
    if ( d->mIsNewVector.contains( taskView ) )
    {
        QString message = i18n( "This document has not been saved yet. Do you want to save it?" );
        QString caption = i18n( "Untitled" );
        int result = KMessageBox::questionYesNoCancel( this, message, caption );
        if ( result == KMessageBox::Cancel )
        {
            return false;
        }
        if ( result == KMessageBox::Yes )
        {
            if ( !saveCurrentTaskView() )
            {
                return false;
            }
        }
        else
        { // result == No
            d->mIsNewVector.remove( d->mIsNewVector.indexOf( taskView ) );
        }
    }
    if ( taskView )
    {
        taskView->save();
        taskView->closeStorage();
    }

    d->mTabWidget->removeTab( d->mTabWidget->currentIndex() );
    d->mSearchWidget->removeTreeWidget( taskView );

    /* emit signals and call slots since currentChanged is not emitted if
    * we close the last tab
    */
    if ( d->mTabWidget->count() == 0 )
    {
        emit currentTaskViewChanged();
        emit setCaption( QString() );
        slotCurrentChanged();
    }

    /* hide the tabbar if there's only one tab left */
    if ( d->mTabWidget->count() < 2 )
    {
        showTabBar( false );
    }

    delete taskView; // removeTab does not delete its widget.
    return true;
}

void TimetrackerWidget::saveFile()
{
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

    // is it an unsaved file?
    if ( d->mIsNewVector.contains( taskView ) )
    {
        saveCurrentTaskView();
    }
    taskView->save();
    // TODO get eventually error message from save and emit
    // a error message signal.
}

void TimetrackerWidget::showTabBar( bool visible )
{
    d->mTabWidget->setTabBarHidden( !visible );
}

void TimetrackerWidget::reconfigureFiles()
{
    kDebug(5970) << "Entering function";
    TaskView *taskView;
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
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
    kDebug(5970) << "Entering TimetrackerWidget::closeAllFiles";
    while ( d->mTabWidget->count() > 0 )
    {
        TaskView *taskView = (TaskView*)( d->mTabWidget->widget( 0 ) );
        d->mTabWidget->setCurrentWidget( taskView );
        taskView->stopAllTimers();
        if ( !( closeFile() ) )
            return false;
    }
    return true;
}

void TimetrackerWidget::slotCurrentChanged()
{
    kDebug() << "entering KTimetrackerWidget::slotCurrentChanged";

    if ( d->mLastView )
    {
        disconnect( d->mLastView, SIGNAL( totalTimesChanged( long, long ) ) );
        disconnect( d->mLastView, SIGNAL( reSetTimes() ) );
        disconnect( d->mLastView, SIGNAL( itemSelectionChanged() ) );
        disconnect( d->mLastView, SIGNAL( updateButtons() ) );
        disconnect( d->mLastView, SIGNAL( setStatusBarText( QString ) ) );
        disconnect( d->mLastView, SIGNAL( timersActive() ) );
        disconnect( d->mLastView, SIGNAL( timersInactive() ) );
        disconnect( d->mLastView, SIGNAL( tasksChanged( const QList< Task* >& ) ),
                this, SIGNAL( tasksChanged( const QList< Task* > & ) ) );
    }

    d->mLastView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

    if ( d->mLastView )
    {
        connect( d->mLastView, SIGNAL( totalTimesChanged( long, long ) ),
            this, SIGNAL( totalTimesChanged( long, long ) ) );
        connect( d->mLastView, SIGNAL( reSetTimes() ),
            this, SIGNAL( reSetTimes() ) );
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
        emit setCaption( d->mLastView->storage()->icalfile() );
    }
    d->mSearchWidget->setEnabled( d->mLastView );
}

void TimetrackerWidget::updateTabs()
{
    kDebug(5970) << "Entering function";
    TaskView *taskView;
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        taskView = (TaskView*)( d->mTabWidget->widget( i ) );
        if ( taskView->activeTasks().count() == 0 )
        {
            d->mTabWidget->setTabTextColor( i, palette().color( QPalette::Foreground ) );
        }
        else
        {
            d->mTabWidget->setTabTextColor( i, Qt::darkGreen );
        }
    }
    kDebug(5970) << "Leaving function";
}

bool TimetrackerWidget::eventFilter( QObject *obj, QEvent *event )
{
    if ( obj == d->mSearchWidget )
    {
        if ( event->type() == QEvent::KeyPress )
        {
            QKeyEvent *keyEvent = static_cast< QKeyEvent* >( event );
            if ( keyEvent->key() == Qt::Key_Enter ||
                keyEvent->key() == Qt::Key_Return )
            {
                if ( !d->mSearchWidget->displayText().isEmpty() ) slotAddTask( d->mSearchWidget->displayText() );
                    return true;
            }
        }
    }
  return QObject::eventFilter( obj, event );
}

void TimetrackerWidget::slotAddTask( const QString &taskName )
{
    TaskView *taskView = (TaskView*)( d->mTabWidget->currentWidget() );
    taskView->addTask( taskName, 0, 0, DesktopList(), 0 );

    d->mSearchWidget->clear();
    d->mTabWidget->setFocus( Qt::OtherFocusReason );
}

void TimetrackerWidget::slotUpdateButtons()
{
    kDebug(5970) << "Entering function";
    Task *item = currentTask();

    d->mActions[ "start" ]->setEnabled( item && !item->isRunning() &&
      !item->isComplete() );
    d->mActions[ "stop" ]->setEnabled( item && item->isRunning() );
    d->mActions[ "delete_task" ]->setEnabled( item );
    d->mActions[ "edit_task" ]->setEnabled( item );
    d->mActions[ "mark_as_complete" ]->setEnabled( item && !item->isComplete() );
    d->mActions[ "mark_as_incomplete" ]->setEnabled( item && item->isComplete() );

    d->mActions[ "new_task" ]->setEnabled( currentTaskView() );
    d->mActions[ "new_sub_task" ]->setEnabled( currentTaskView() && currentTaskView()->count() );
    d->mActions[ "focustracking" ]->setEnabled( currentTaskView() );
    d->mActions[ "focustracking" ]->setChecked( currentTaskView() &&
        currentTaskView()->isFocusTrackingActive() );
    d->mActions[ "start_new_session" ]->setEnabled( currentTaskView() );
    d->mActions[ "edit_history" ]->setEnabled( currentTaskView() );
    d->mActions[ "reset_all_times" ]->setEnabled( currentTaskView() );
    d->mActions[ "export_times" ]->setEnabled( currentTaskView() );
    d->mActions[ "export_history" ]->setEnabled( currentTaskView() );
    d->mActions[ "import_planner" ]->setEnabled( currentTaskView() );
    d->mActions[ "file_save" ]->setEnabled( currentTaskView() );
    d->mActions[ "file_close" ]->setEnabled( currentTaskView() );
    kDebug(5970) << "Leaving function";
}

void TimetrackerWidget::showSettingsDialog()
{
    kDebug(5970) << "Entering function";
    /* show main window b/c if this method was started from tray icon and the window
        is not visible the application quits after accepting the settings dialog.
    */
    window()->show();
    KTimeTrackerConfigDialog *dialog = new KTimeTrackerConfigDialog( i18n( "Settings" ), this);
    dialog->exec();
    delete dialog;

    showSearchBar( !KTimeTrackerSettings::configPDA() );
    reconfigureFiles();
}

//BEGIN wrapper slots
void TimetrackerWidget::startCurrentTimer()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->startCurrentTimer();
    }
}

void TimetrackerWidget::stopCurrentTimer()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->stopCurrentTimer();
    }
}

void TimetrackerWidget::stopAllTimers( const QDateTime &when )
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->stopAllTimers( when );
    }
}

void TimetrackerWidget::newTask()
{
    if ( d->mTabWidget->count() == 0 )
        newFile() ;

    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->newTask();
    }
}

void TimetrackerWidget::newSubTask()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->newSubTask();
    }
}

void TimetrackerWidget::editTask()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->editTask();
    }
}

void TimetrackerWidget::deleteTask()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->deleteTask();
    }
}

void TimetrackerWidget::markTaskAsComplete()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->markTaskAsComplete();
    }
}

void TimetrackerWidget::markTaskAsIncomplete()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->markTaskAsIncomplete();
    }
}

void TimetrackerWidget::exportcsvFile()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->exportcsvFile();
    }
}

void TimetrackerWidget::exportcsvHistory()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->exportcsvHistory();
    }
}

void TimetrackerWidget::importPlanner( const QString &fileName )
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->importPlanner( fileName );
    }
}

void TimetrackerWidget::startNewSession()
{
    if ( d->mTabWidget->currentWidget() )
    {
        qobject_cast< TaskView* >( d->mTabWidget->currentWidget() )->startNewSession();
    }
}

void TimetrackerWidget::editHistory()
{
    // historydialog is the new historydialog, but the EditHiStoryDiaLog exists as well.
    // historydialog can be edited with qtcreator and qtdesigner, EditHiStoryDiaLog can not.
    if ( d->mTabWidget->currentWidget() )
    {
        historydialog *dlg = new historydialog( qobject_cast< TaskView* >( d->mTabWidget->currentWidget() ) );
        if (currentTaskView()->storage()->rawevents().count()!=0) dlg->exec();
        else KMessageBox::information(0, i18nc("@info in message box", "There is no history yet. Start and stop a task and you will have an entry in your history."));
    }
}

void TimetrackerWidget::resetAllTimes()
{
    if ( d->mTabWidget->currentWidget() )
    {
        if ( KMessageBox::warningContinueCancel( this,
            i18n( "Do you really want to reset the time to zero for all tasks? This will delete the entire history." ),
            i18n( "Confirmation Required" ), KGuiItem( i18n( "Reset All Times" ) ) ) == KMessageBox::Continue )
        currentTaskView()->resetTimeForAllTasks();
    }
}

void TimetrackerWidget::focusTracking()
{
    if ( d->mTabWidget->currentWidget() )
    {
        currentTaskView()->toggleFocusTracking();
        d->mActions[ "focustracking" ]->setChecked(
        currentTaskView()->isFocusTrackingActive() );
    }
}

void TimetrackerWidget::slotSearchBar()
{
    bool currentVisible = KTimeTrackerSettings::self()->showSearchBar();
    KTimeTrackerSettings::self()->setShowSearchBar( !currentVisible );
    d->mActions[ "searchbar" ]->setChecked( !currentVisible );
    showSearchBar( !currentVisible );
}
//END

/** \defgroup dbus slots ‘‘dbus slots’’ */
/* @{ */
QString TimetrackerWidget::version() const
{
    return KTIMETRACKER_VERSION;
}

QStringList TimetrackerWidget::taskIdsFromName( const QString &taskName ) const
{
    QStringList result;

    for ( int i = 0; i < d->mTabWidget->count(); ++i)
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->name() == taskName )
            {
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

    if ( taskView )
    {
        taskView->addTask( taskName, 0, 0, DesktopList(), 0 );
    }
}

void TimetrackerWidget::addSubTask( const QString& taskName, const QString &taskId )
{
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

    if ( taskView )
    {
        taskView->addTask( taskName, 0, 0, DesktopList(), taskView->task( taskId) );
        taskView->refresh();
    }
}

void TimetrackerWidget::deleteTask( const QString &taskId )
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i)
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );

        if ( !taskView ) continue;

        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->uid() == taskId )
            {
                taskView->deleteTaskBatch( task );
            }
            ++it;
        }
    }
}

void TimetrackerWidget::setPercentComplete( const QString &taskId, int percent )
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i)
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;

        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->uid() == taskId )
            {
                task->setPercentComplete( percent, taskView->storage() );
            }
            ++it;
        }
    }
}

int TimetrackerWidget::bookTime( const QString &taskId, const QString &dateTime, int minutes )
{
    QDate startDate;
    QTime startTime;
    QDateTime startDateTime;
    Task *task = 0, *t = 0;

    if ( minutes <= 0 ) return KTIMETRACKER_ERR_INVALID_DURATION;

    // Find task
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;

        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            t = static_cast< Task* >( *it );
            if ( t && t->uid() == taskId )
            {
                task = t;
                break;
            }
            ++it;
        }
        if ( task ) break;
    }

    if ( !task ) return KTIMETRACKER_ERR_UID_NOT_FOUND;

    // Parse datetime
    startDate = QDate::fromString( dateTime, Qt::ISODate );

    if ( dateTime.length() > 10 )
    { // "YYYY-MM-DD".length() = 10
        startTime = QTime::fromString( dateTime, Qt::ISODate );
    } else startTime = QTime( 12, 0 );

    if ( startDate.isValid() && startTime.isValid() )
    {
        startDateTime = QDateTime( startDate, startTime );
    } else return KTIMETRACKER_ERR_INVALID_DATE;

    // Update task totals (session and total) and save to disk
    task->changeTotalTimes( task->sessionTime() + minutes,
                          task->totalTime() + minutes );
    if ( !( task->taskView()->storage()->bookTime( task,
                                                 startDateTime,
                                                 minutes * 60 ) ) )
        return KTIMETRACKER_ERR_GENERIC_SAVE_FAILED;

    return 0;
}

int TimetrackerWidget::changeTime( const QString &taskId, int minutes )
{
    int result=0;
    QDate startDate;
    QTime startTime;
    QDateTime startDateTime;
    Task *task = 0, *t = 0;

    if ( minutes <= 0 ) return KTIMETRACKER_ERR_INVALID_DURATION;

    // Find task
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;

        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            t = static_cast< Task* >( *it );
            if ( t && t->uid() == taskId )
            {
                task = t;
                break;
            }
            ++it;
        }
        if ( task ) break;
    }

    if ( !task ) result=KTIMETRACKER_ERR_UID_NOT_FOUND;
    else task->changeTime(minutes, task->taskView()->storage());
    return result;
}

QString TimetrackerWidget::error( int errorCode ) const
{
    switch ( errorCode )
    {
        case KTIMETRACKER_ERR_GENERIC_SAVE_FAILED:
            return i18n( "Save failed, most likely because the file could not be locked." );
        case KTIMETRACKER_ERR_COULD_NOT_MODIFY_RESOURCE:
            return i18n( "Could not modify calendar resource." );
        case KTIMETRACKER_ERR_MEMORY_EXHAUSTED:
            return i18n( "Out of memory--could not create object." );
        case KTIMETRACKER_ERR_UID_NOT_FOUND:
            return i18n( "UID not found." );
        case KTIMETRACKER_ERR_INVALID_DATE:
            return i18n( "Invalidate date--format is YYYY-MM-DD." );
        case KTIMETRACKER_ERR_INVALID_TIME:
            return i18n( "Invalid time--format is YYYY-MM-DDTHH:MM:SS." );
        case KTIMETRACKER_ERR_INVALID_DURATION:
            return i18n( "Invalid task duration--must be greater than zero." );
        default:
            return i18n( "Invalid error number: %1", errorCode );
    }
}

bool TimetrackerWidget::isIdleDetectionPossible() const
{
    bool result;
    IdleTimeDetector *idletimedetector1=new IdleTimeDetector(50);
    result=idletimedetector1->isIdleDetectionPossible();
    delete idletimedetector1;
    return result;
}

int TimetrackerWidget::totalMinutesForTaskId( const QString &taskId ) const
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->uid() == taskId )
            {
                return task->totalTime();
            }
        ++it;
        }
    }
    return -1;
}

void TimetrackerWidget::startTimerFor( const QString &taskId )
{
    kDebug();
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->uid() == taskId )
            {
                taskView->startTimerFor( task );
                return;
            }

            ++it;
        }
    }
}

bool TimetrackerWidget::startTimerForTaskName( const QString &taskName )
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->name() == taskName )
            {
                taskView->startTimerFor( task );
                return true;
            }
            ++it;
        }
    }
    return false;
}


bool TimetrackerWidget::stopTimerForTaskName( const QString &taskName )
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;

        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );

            if ( task && task->name() == taskName )
            {
                taskView->stopTimerFor( task );
                return true;
            }
        ++it;
        }
    }
    return false;
}


void TimetrackerWidget::stopTimerFor( const QString &taskId )
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->uid() == taskId )
            {
                taskView->stopTimerFor( task );
                return;
            }

            ++it;
        }
    }
}

void TimetrackerWidget::stopAllTimersDBUS()
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        taskView->stopAllTimers();
    }
}

QString TimetrackerWidget::exportCSVFile( const QString &filename,
                                          const QString &from,
                                          const QString &to, int type,
                                          bool decimalMinutes,
                                          bool allTasks,
                                          const QString &delimiter,
                                          const QString &quote )
{
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );

    if ( !taskView ) return "";
    ReportCriteria rc;
    rc.url = filename;
    rc.from = QDate::fromString( from );
    if ( rc.from.isNull() )
        rc.from = QDate::fromString( from, Qt::ISODate );
    rc.to = QDate::fromString( to );
    if ( rc.to.isNull() )
        rc.to = QDate::fromString( to, Qt::ISODate );
    rc.reportType = ( ReportCriteria::REPORTTYPE )type;
    rc.decimalMinutes = decimalMinutes;
    rc.allTasks = allTasks;
    rc.delimiter = delimiter;
    rc.quote = quote;

    return taskView->report( rc );
}

void TimetrackerWidget::importPlannerFile( const QString &filename )
{
    TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->currentWidget() );
    if ( !taskView ) return;
    taskView->importPlanner( filename );
}

bool TimetrackerWidget::isActive( const QString &taskId ) const
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );

            if ( task && task->uid() == taskId )
            {
                return task->isRunning();
            }
            ++it;
        }
    }
  return false;
}

bool TimetrackerWidget::isTaskNameActive( const QString &taskName ) const
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            Task *task = static_cast< Task* >( *it );
            if ( task && task->name() == taskName )
            {
                return task->isRunning();
            }
        ++it;
        }
    }
  return false;
}

QStringList TimetrackerWidget::tasks() const
{
    QStringList result;
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        QTreeWidgetItemIterator it( taskView );
        while ( *it )
        {
            result << static_cast< Task* >( *it )->name();
            ++it;
        }
    }
    return result;
}

QStringList TimetrackerWidget::activeTasks() const
{
    QStringList result;
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;
        for ( int j = 0; j < taskView->count(); ++j )
        {
            if ( taskView->itemAt( j )->isRunning() )
            {
                result << taskView->itemAt( j )->name();
            }
        }
    }
    return result;
}

void TimetrackerWidget::saveAll()
{
    for ( int i = 0; i < d->mTabWidget->count(); ++i )
    {
        TaskView *taskView = qobject_cast< TaskView* >( d->mTabWidget->widget( i ) );
        if ( !taskView ) continue;

        // is it an unsaved file?
        if ( d->mIsNewVector.contains( taskView ) )
        {
            saveCurrentTaskView();
        }
        taskView->save();
    }
}

bool TimetrackerWidget::event ( QEvent * event ) // inherited from QWidget
{
    if (event->type()==QEvent::QueryWhatsThis)
    {
        if ( d->mLastView->count() == 0 )
            setWhatsThis( i18n("This is ktimetracker, KDE's program to help you track your time. Best, start with creating your first task - enter it into the field where you see \"search or add task\".") );
        else setWhatsThis( i18n("You have already created a task. You can now start and stop timing") );
    }
    return QWidget::event(event);
}

void TimetrackerWidget::quit()
{
    kDebug(5970) << "Entering TimetrackerWidget::quit";
    if ( closeAllFiles() )
    {
        kapp->quit();
    }
}
// END of dbus slots group
/* @} */
#include "timetrackerwidget.moc"
