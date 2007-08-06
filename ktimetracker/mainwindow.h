/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
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

#ifndef KARM_MAIN_WINDOW_H
#define KARM_MAIN_WINDOW_H

#include <KParts/MainWindow>

#include "karmerrors.h"
#include "reportcriteria.h"

class KAccel;
class KAccelMenuWatch;
class KRecentFilesAction;
class KarmTray;
class QPoint;
class QString;

class Task;
class TaskView;
class TimetrackerWidget;

/**
 * Main window to tie the application together.
 */

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  private:
    void             makeMenus();
    QString          _hasTask( Task* task, const QString &taskname ) const;
    Task*            _hasUid( Task* task, const QString &uid ) const;

    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    long             _totalSum;
    long             _sessionSum;
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
    KAction*         actionKeyBindings;
    KAction*         actionNew;
    KAction*         actionNewSub;
    KAction*         actionedithistory;
    KAction*         actionFocusTracking;
    KAction*         actionResetAll;
    KAction*         actionStartNewSession;
    KAction*         actionExportTimes;
    KAction*         actionExportHistory;
    KAction*         actionImportPlanner;
    KAction*         actionClose;
    KAction*         actionPrint;
    KAction*         actionSave;
    KAction*         actionSearchBar;
    KRecentFilesAction* actionRecentFiles;
    QString          m_error[ KARM_MAX_ERROR_NO + 1 ];

    TimetrackerWidget *mainWidget;

    friend class KarmTray;

  public:
    MainWindow( const QString &icsfile = "" );
    virtual ~MainWindow();

    // FIXME consistent method names, e.g. getError, stoptimefor, ...
    /** DCOP function: print out version */
//     QString version() const;
//     QString taskIdFromName( const QString &taskName ) const;
//     /** @reimp from KarmDCOPIface::addTask */
//     int addTask( const QString &storage );
//     /** @reimp from KarmDCOPIface::setPerCentComplete */
//     QString setPerCentComplete( const QString& taskName, int PerCent );
//     /** @reimp from KarmDCOPIface::bookTime */
//     int bookTime( const QString& taskId, const QString& iso8601StartDateTime, long durationInMinutes );
//     /** @reimp from KarmDCOPIface::getError */
//     QString getError( int karmErrorNumber ) const;
//     /** Delivers the total minutes for taskId */
//     int totalMinutesForTaskId( const QString& taskId );
//     /** starts taskname's timer */
//     QString starttimerfor( const QString &taskname );
//     QString stoptimerfor( const QString &taskname );
//     QString stopalltimers();
//     QString deletetodo();
//     /** Delivers true if ktimetracker asks before deleting a task. This is stored as a config setting. */
//     bool    getpromptdelete();
//     QString setpromptdelete( bool prompt );
//     QString exportcsvfile( const QString &filename, const QString &from, 
//                            const QString &to, int type, bool decimalMinutes, 
//                            bool allTasks, const QString &delimiter, 
//                            const QString &quote );
//     QString importplannerfile( const QString &filename );
//     QStringList getActiveTasks();
//     QStringList getTasks();
//     bool isActive( const QString &taskName );

  public Q_SLOTS:
    void setStatusBar( const QString& );
    /** Quit ktimetracker (what else...) */
    void quit();
    /** Save the calendar */
    bool save();
  protected Q_SLOTS:
    void keyBindings();
    void startNewSession();
    void resetAllTimes();
    void updateTime( long, long );
    void updateStatusBar();
    void exportcsvHistory();
    void slotedithistory();
    void print();
    void slotSelectionChanged();
    void taskViewCustomContextMenuRequested( const QPoint& );
    void enableStopAll();
    void disableStopAll();
    void slotFocusTracking();
    void openFile();
    void openFile( const KUrl & );
    void showSettingsDialog();
    void slotSearchBar();
//    void timeLoggingChanged( bool on );

  protected:
    void startStatusBar();
    virtual void saveProperties( KConfigGroup& );
    virtual void readProperties( const KConfigGroup& );
    void saveGeometry();
    void loadGeometry();
    bool queryClose();

};

#endif // KARM_MAIN_WINDOW_H
