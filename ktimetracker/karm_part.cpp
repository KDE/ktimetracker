
#include "kaccelmenuwatch.h"
#include "karm_part.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include <kaccel.h>

#include <kinstance.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klocale.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qmultilineedit.h>

karmPart::karmPart( QWidget *parentWidget, const char *widgetName,
                                  QObject *parent, const char *name )
    : KParts::ReadWritePart(parent, name), 
    _accel     ( new KAccel( parentWidget ) ),
    _watcher   ( new KAccelMenuWatch( _accel, parentWidget ) )
{
    // we need an instance
    setInstance( karmPartFactory::instance() );

    // this should be your custom internal widget
    _taskView = new TaskView( parentWidget, widgetName );

    // setup PreferenceDialog.
    _preferences = Preferences::instance();

   // notify the part that this is our internal widget
    setWidget(_taskView);

    // create our actions
    KStdAction::open(this, SLOT(fileOpen()), actionCollection());
    KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
    KStdAction::save(this, SLOT(save()), actionCollection());

    makeMenus();

  _watcher->updateMenus();

  // connections

  connect( _taskView, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( _taskView, SIGNAL( selectionChanged ( QListViewItem * )),
           this, SLOT(slotSelectionChanged()));
  connect( _taskView, SIGNAL( updateButtons() ),
           this, SLOT(slotSelectionChanged()));

  // Setup context menu request handling
  connect( _taskView,
           SIGNAL( contextMenuRequested( QListViewItem*, const QPoint&, int )),
           this,
           SLOT( contextMenuRequest( QListViewItem*, const QPoint&, int )));

  _tray = new KarmTray( this );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( _taskView, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( _taskView, SIGNAL( timersActive() ), this,  SLOT( enableStopAll() ));
  connect( _taskView, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( _taskView, SIGNAL( timersInactive() ),  this,  SLOT( disableStopAll()));
  connect( _taskView, SIGNAL( tasksChanged( QPtrList<Task> ) ),
                      _tray, SLOT( updateToolTip( QPtrList<Task> ) ));

  _taskView->load();

  // Everything that uses Preferences has been created now, we can let it
  // emit its signals
  _preferences->emitSignals();
  slotSelectionChanged();

    // set our XML-UI resource file
    setXMLFile("karmui.rc");

    // we are read-write by default
    setReadWrite(true);

    // we are not modified since we haven't done anything yet
    setModified(false);
}

karmPart::~karmPart()
{
}

void karmPart::slotSelectionChanged()
{
  Task* item= _taskView->current_item();
  actionDelete->setEnabled(item);
  actionEdit->setEnabled(item);
  actionStart->setEnabled(item && !item->isRunning() && !item->isComplete());
  actionStop->setEnabled(item && item->isRunning());
  actionMarkAsComplete->setEnabled(item && !item->isComplete());
  actionMarkAsIncomplete->setEnabled(item && item->isComplete());
}

void karmPart::makeMenus()
{
  KAction
    *actionKeyBindings,
    *actionNew,
    *actionNewSub;

  (void) KStdAction::quit(  this, SLOT( quit() ),  actionCollection());
  (void) KStdAction::print( this, SLOT( print() ), actionCollection());
  actionKeyBindings = KStdAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );
  actionPreferences = KStdAction::preferences(_preferences,
      SLOT(showDialog()),
      actionCollection() );
  (void) KStdAction::save( this, SLOT( save() ), actionCollection() );
  KAction* actionStartNewSession = new KAction( i18n("Start &New Session"),
      0,
      this,
      SLOT( startNewSession() ),
      actionCollection(),
      "start_new_session");
  KAction* actionResetAll = new KAction( i18n("&Reset All Times"),
      0,
      this,
      SLOT( resetAllTimes() ),
      actionCollection(),
      "reset_all_times");
  actionStart = new KAction( i18n("&Start"),
      QString::fromLatin1("1rightarrow"), Key_S,
      _taskView,
      SLOT( startCurrentTimer() ), actionCollection(),
      "start");
  actionStop = new KAction( i18n("S&top"),
      QString::fromLatin1("stop"), 0,
      _taskView,
      SLOT( stopCurrentTimer() ), actionCollection(),
      "stop");
  actionStopAll = new KAction( i18n("Stop &All Timers"),
      Key_Escape,
      _taskView,
      SLOT( stopAllTimers() ), actionCollection(),
      "stopAll");
  actionStopAll->setEnabled(false);

  actionNew = new KAction( i18n("&New..."),
      QString::fromLatin1("filenew"), CTRL+Key_N,
      _taskView,
      SLOT( newTask() ), actionCollection(),
      "new_task");
  actionNewSub = new KAction( i18n("New &Subtask..."),
      QString::fromLatin1("kmultiple"), CTRL+ALT+Key_N,
      _taskView,
      SLOT( newSubTask() ), actionCollection(),
      "new_sub_task");
  actionDelete = new KAction( i18n("&Delete"),
      QString::fromLatin1("editdelete"), Key_Delete,
      _taskView,
      SLOT( deleteTask() ), actionCollection(),
      "delete_task");
  actionEdit = new KAction( i18n("&Edit..."),
      QString::fromLatin1("edit"), CTRL + Key_E,
      _taskView,
      SLOT( editTask() ), actionCollection(),
      "edit_task");
//  actionAddComment = new KAction( i18n("&Add Comment..."),
//      QString::fromLatin1("document"),
//      CTRL+ALT+Key_E,
//      _taskView,
//      SLOT( addCommentToTask() ),
//      actionCollection(),
//      "add_comment_to_task");
  actionMarkAsComplete = new KAction( i18n("&Mark as Complete"),
      QString::fromLatin1("document"),
      CTRL+Key_M,
      _taskView,
      SLOT( markTaskAsComplete() ),
      actionCollection(),
      "mark_as_complete");
  actionMarkAsIncomplete = new KAction( i18n("&Mark as Incomplete"),
      QString::fromLatin1("document"),
      CTRL+Key_M,
      _taskView,
      SLOT( markTaskAsIncomplete() ),
      actionCollection(),
      "mark_as_incomplete");
  actionClipTotals = new KAction( i18n("&Copy Totals to Clipboard"),
      QString::fromLatin1("klipper"),
      CTRL+Key_C,
      _taskView,
      SLOT( clipTotals() ),
      actionCollection(),
      "clip_totals");
  actionClipHistory = new KAction( i18n("Copy &History to Clipboard"),
      QString::fromLatin1("klipper"),
      CTRL+ALT+Key_C,
      _taskView,
      SLOT( clipHistory() ),
      actionCollection(),
      "clip_history");

  new KAction( i18n("Import &Legacy Flat File..."), 0,
      _taskView, SLOT(loadFromFlatFile()), actionCollection(),
      "import_flatfile");
  new KAction( i18n("&Export to CSV File..."), 0,
      _taskView, SLOT(exportcsvFile()), actionCollection(),
      "export_csvfile");
  new KAction( i18n("Export &History to CSV File..."), 0,
      this, SLOT(exportcsvHistory()), actionCollection(),
      "export_csvhistory");
  new KAction( i18n("Import Tasks From &Planner..."), 0,
      _taskView, SLOT(importPlanner()), actionCollection(),
      "import_planner");  

/*
  new KAction( i18n("Import E&vents"), 0,
                            _taskView,
                            SLOT( loadFromKOrgEvents() ), actionCollection(),
                            "import_korg_events");
  */

  // Tool tops must be set after the createGUI.
  actionKeyBindings->setToolTip( i18n("Configure key bindings") );
  actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which is specific to karm") );

  actionStartNewSession->setToolTip( i18n("Start a new session") );
  actionStartNewSession->setWhatsThis( i18n("This will reset the session time "
                                            "to 0 for all tasks, to start a "
                                            "new session, without affecting "
                                            "the totals.") );
  actionResetAll->setToolTip( i18n("Reset all times") );
  actionResetAll->setWhatsThis( i18n("This will reset the session and total "
                                     "time to 0 for all tasks, to restart from "
                                     "scratch.") );

  actionStart->setToolTip( i18n("Start timing for selected task") );
  actionStart->setWhatsThis( i18n("This will start timing for the selected "
                                  "task.\n"
                                  "It is even possible to time several tasks "
                                  "simultaneously.\n\n"
                                  "You may also start timing of a tasks by "
                                  "double clicking the left mouse "
                                  "button on a given task. This will, however, "
                                  "stop timing of other tasks."));

  actionStop->setToolTip( i18n("Stop timing of the selected task") );
  actionStop->setWhatsThis( i18n("Stop timing of the selected task") );

  actionStopAll->setToolTip( i18n("Stop all of the active timers") );
  actionStopAll->setWhatsThis( i18n("Stop all of the active timers") );

  actionNew->setToolTip( i18n("Create new top level task") );
  actionNew->setWhatsThis( i18n("This will create a new top level task.") );

  actionDelete->setToolTip( i18n("Delete selected task") );
  actionDelete->setWhatsThis( i18n("This will delete the selected task and "
                                   "all its subtasks.") );

  actionEdit->setToolTip( i18n("Edit name or times for selected task") );
  actionEdit->setWhatsThis( i18n("This will bring up a dialog box where you "
                                 "may edit the parameters for the selected "
                                 "task."));
  //actionAddComment->setToolTip( i18n("Add a comment to a task") );
  //actionAddComment->setWhatsThis( i18n("This will bring up a dialog box where "
  //                                     "you can add a comment to a task. The "
  //                                     "comment can for instance add information on what you "
  //                                     "are currently doing. The comment will "
  //                                     "be logged in the log file."));
  actionClipTotals->setToolTip(i18n("Copy task totals to clipboard"));
  actionClipHistory->setToolTip(i18n("Copy time card history to clipboard."));

  slotSelectionChanged();
}

void karmPart::setReadWrite(bool rw)
{
    // notify your internal widget of the read-write state
    if (rw)
        connect(_taskView, SIGNAL(textChanged()),
                this,     SLOT(setModified()));
    else
    {
        disconnect(_taskView, SIGNAL(textChanged()),
                   this,     SLOT(setModified()));
    }

    ReadWritePart::setReadWrite(rw);
}

void karmPart::setModified(bool modified)
{
    // get a handle on our Save action and make sure it is valid
    KAction *save = actionCollection()->action(KStdAction::stdName(KStdAction::Save));
    if (!save)
        return;

    // if so, we either enable or disable it based on the current
    // state
    if (modified)
        save->setEnabled(true);
    else
        save->setEnabled(false);

    // in any event, we want our parent to do it's thing
    ReadWritePart::setModified(modified);
}

bool karmPart::openFile()
{
    // m_file is always local so we can use QFile on it
    _taskView->load(m_file);

    // just for fun, set the status bar
    emit setStatusBarText( m_url.prettyURL() );

    return true;
}

bool karmPart::saveFile()
{
    // if we aren't read-write, return immediately
    if (isReadWrite() == false)
        return false;

    // m_file is always local, so we use QFile
    QFile file(m_file);
    if (file.open(IO_WriteOnly) == false)
        return false;

    // use QTextStream to dump the text to the file
    QTextStream stream(&file);

    file.close();

    return true;
}

void karmPart::fileOpen()
{
    // this slot is called whenever the File->Open menu is selected,
    // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
    // button is clicked
    QString file_name = KFileDialog::getOpenFileName();

    if (file_name.isEmpty() == false)
        openURL(file_name);
}

void karmPart::fileSaveAs()
{
    // this slot is called whenever the File->Save As menu is selected,
    QString file_name = KFileDialog::getSaveFileName();
    if (file_name.isEmpty() == false)
        saveAs(file_name);
}


// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>
#include <klocale.h>

KInstance*  karmPartFactory::s_instance = 0L;
KAboutData* karmPartFactory::s_about = 0L;

karmPartFactory::karmPartFactory()
    : KParts::Factory()
{
}

karmPartFactory::~karmPartFactory()
{
    delete s_instance;
    delete s_about;

    s_instance = 0L;
}

KParts::Part* karmPartFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
                                                        QObject *parent, const char *name,
                                                        const char *classname, const QStringList &args )
{
    // Create an instance of our Part
    karmPart* obj = new karmPart( parentWidget, widgetName, parent, name );

    // See if we are to be read-write or not
    if (QCString(classname) == "KParts::ReadOnlyPart")
        obj->setReadWrite(false);

    return obj;
}

KInstance* karmPartFactory::instance()
{
    if( !s_instance )
    {
        s_about = new KAboutData("karmpart", I18N_NOOP("karmPart"), "0.1");
        s_about->addAuthor("Thorsten Staerk", 0, "thorsten@staerk.de");
        s_instance = new KInstance(s_about);
    }
    return s_instance;
}

extern "C"
{
    KDE_EXPORT void* init_libkarmpart()
    {
	KGlobal::locale()->insertCatalogue("karm");
        return new karmPartFactory;
    }
}

#include <qpopupmenu.h>
#include "karm_part.moc"
