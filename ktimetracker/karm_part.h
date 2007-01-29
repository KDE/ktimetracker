/*
 *     Copyright (C) 2007 the ktimetracker developers
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

#ifndef _KARMPART_H_
#define _KARMPART_H_

#include <kparts/part.h>
#include "karmerrors.h"
#include <kparts/factory.h>
#include "reportcriteria.h"
#include <Q3ListView>

class KAccel;
class KAccelMenuWatch;
class KarmTray;
class QWidget;
class QPainter;
class KUrl;

class Preferences;
class Task;
class TaskView;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Thorsten Staerk <kde at staerk dot de>
 * @version 0.1
 */
class karmPart : public KParts::ReadWritePart
{
  Q_OBJECT

  private:
    void             makeMenus();
    QString          _hastodo( Task* task, const QString &taskname ) const;
    QString          _hasTask( Task* task, const QString &taskname ) const;
    Task*            _hasUid( Task* task, const QString &uid ) const;

    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    TaskView*        _taskView;
    Preferences*     _preferences;
    KarmTray*        _tray;
    KAction*         actionStart;
    KAction*         actionStop;
    KAction*         actionStopAll;
    KAction*         actionDelete;
    KAction*         actionEdit;
//    KAction* actionAddComment;
    KAction*         actionMarkAsComplete;
    KAction*         actionMarkAsIncomplete;
    KAction*         actionPreferences;
    KAction*         actionClipTotals;
    KAction*         actionClipHistory;
    QString          m_error[ KARM_MAX_ERROR_NO + 1 ];

    friend class KarmTray;

public:
    karmPart(QWidget *parentWidget, QObject *parent);

    void contextMenuRequest( Q3ListViewItem*, const QPoint& point, int );
    // DCOP
    void quit();
    virtual bool save();
    QString version() const;
    QString taskIdFromName( const QString &taskName ) const;
    /** @reimp from KarmDCOPIface */
    int addTask( const QString &taskName );
    /** @reimp from KarmDCOPIface */
    QString setPerCentComplete( const QString& taskName, int PerCent );
    /** @reimp from KarmDCOPIface */
    int bookTime( const QString& taskId, const QString& iso8601StartDateTime, long durationInMinutes );
    /** @reimp from KarmDCOPIface */
    QString getError( int karmErrorNumber ) const;
    int totalMinutesForTaskId( const QString& taskId );
    QString starttimerfor( const QString &taskname );
    QString stoptimerfor( const QString &taskname );
    QString stopalltimers();
    QString deletetodo();
    bool    getpromptdelete();
    QString setpromptdelete( bool prompt );
    QString exportcsvfile( QString filename, QString from, QString to, int type = 0, bool decimalMinutes=true, bool allTasks=true, QString delimiter="r", QString quote="q" );
    QString importplannerfile( QString filename );

    virtual ~karmPart();

    /**
     * This is a virtual function inherited from KParts::ReadWritePart.  
     * A shell will use this to inform this Part if it should act
     * read-only
     */
    virtual void setReadWrite(bool rw);

    /**
     * Reimplemented to disable and enable Save action
     */
    virtual void setModified(bool modified);

protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();

    /**
     * This must be implemented by each read-write part
     */
    virtual bool saveFile();

protected slots:
    void fileOpen();
    void fileSaveAs();
    void slotSelectionChanged();
    void startNewSession(); 

public slots:
   void setStatusBar(const QString & qs);

};

class KComponentData;
class KAboutData;

class karmPartFactory : public KParts::Factory
{
    Q_OBJECT
public:
    karmPartFactory();
    virtual ~karmPartFactory();
    virtual KParts::Part* createPartObject( QWidget *parentWidget, QObject *parent,
                                            const char *classname, const QStringList &args );
    static const KComponentData &componentData();
 
private:
    static KComponentData *s_instance;
    static KAboutData* s_about;
};

#endif // _KARMPART_H_
