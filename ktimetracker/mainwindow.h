#ifndef KARM_MAIN_WINDOW_H
#define KARM_MAIN_WINDOW_H

#include <kparts/mainwindow.h>
#include <karmdcopiface.h>

class KAccel;
class KAccelMenuWatch;
class KDialogBase;
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
    KAccel          *_accel;
    KAccelMenuWatch *_watcher;
    TaskView        *_taskView;
    long            _totalSum;
    long            _sessionSum;
    Preferences     *_preferences;
    KarmTray        *_tray;

  public:
    MainWindow( const QString &icsfile = "" );
    virtual ~MainWindow();

    // DCOP
    QString version() const;
    QString hastodo( const QString &storage ) const;
    QString addtodo( const QString &storage );

  public slots:
    void quit();

  protected slots:
    void keyBindings();
    void startNewSession();
    void resetAllTimes();
    void updateTime( long, long );
    void updateStatusBar();
    void save();
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

  private:
    void makeMenus();
    QString _hastodo( Task* task, const QString &taskname ) const;

    KDialogBase *dialog;
    KAction* actionStart;
    KAction* actionStop;
    KAction* actionStopAll;
    KAction* actionDelete;
    KAction* actionEdit;
//    KAction* actionAddComment;
    KAction* actionMarkAsComplete;
    KAction* actionPreferences;
    KAction* actionClipTotals;
    KAction* actionClipHistory;

    friend class KarmTray;
};

#endif // KARM_MAIN_WINDOW_H
