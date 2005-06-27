#ifndef KARM_MAIN_WINDOW_H
#define KARM_MAIN_WINDOW_H

#include <kparts/mainwindow.h>

#include "karmerrors.h"
#include <karmdcopiface.h>

class KAccel;
class KAccelMenuWatch;
class KarmTray;
class QListViewItem;
class QPoint;
class QString;

class Preferences;
class PrintDialog;
class Task;
class TaskView;

/**
 * Main window to tie the application together.
 */

class MainWindow : public KParts::MainWindow, virtual public KarmDCOPIface
{
  Q_OBJECT

  private:
    void             makeMenus();
    QString          _hasTask( Task* task, const QString &taskname ) const;
    Task*            _hasUid( Task* task, const QString &uid ) const;

    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    TaskView*        _taskView;
    long             _totalSum;
    long             _sessionSum;
    Preferences*     _preferences;
    KarmTray*        _tray;
    KAction*         actionStart;
    KAction*         actionStop;
    KAction*         actionStopAll;
    KAction*         actionDelete;
    KAction*         actionEdit;
    KAction*         actionMarkAsComplete;
    KAction*         actionMarkAsIncomplete;
    KAction*         actionPreferences;
    KAction*         actionClipTotals;
    KAction*         actionClipHistory;
    QString          m_error[ KARM_MAX_ERROR_NO ];

    friend class KarmTray;

  //private:

    //KDialogBase *dialog;



  public:
    MainWindow( const QString &icsfile = "" );
    virtual ~MainWindow();

    // DCOP
    QString version() const;
    QString taskIdFromName( const QString &taskName ) const;
    /** @reimp from KarmDCOPIface::addTask */
    int addTask( const QString &storage );
    /** @reimp from KarmDCOPIface::bookTime */
    int bookTime( const QString& taskId, const QString& iso8601StartDateTime, durationInMinutes minutes );
    /** @reimp from KarmDCOPIface::getError */
    QString getError( int karmErrorNumber ) const;
    int totalMinutesForTaskId( const QString& taskId );
    QString starttimerfor( const QString &taskname );
    QString stoptimerfor( const QString &taskname );
    QString deletetodo();
    bool    getpromptdelete();
    QString setpromptdelete( bool prompt );
    QString importplannerfile( QString filename );

  public slots:
    void quit();

  protected slots:
    void keyBindings();
    void startNewSession();
    void resetAllTimes();
    void updateTime( long, long );
    void updateStatusBar();
    bool save();
    void exportcsvHistory();
    void print();
    void slotSelectionChanged();
    void contextMenuRequest( QListViewItem*, const QPoint&, int );
    void enableStopAll();
    void disableStopAll();
//    void timeLoggingChanged( bool on );

  protected:
    void startStatusBar();
    virtual void saveProperties( KConfig* );
    virtual void readProperties( KConfig* );
    void saveGeometry();
    void loadGeometry();
    bool queryClose();

};

#endif // KARM_MAIN_WINDOW_H
