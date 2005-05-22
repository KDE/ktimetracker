#ifndef _KARMPART_H_
#define _KARMPART_H_

#include <kparts/part.h>
#include <kparts/factory.h>

class KAccel;
class KAccelMenuWatch;
class KarmTray;
class QWidget;
class QPainter;
class KURL;

class Preferences;
class Task;
class TaskView;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Thorsten Staerk <thorsten@staerk.de>
 * @version 0.1
 */
class karmPart : public KParts::ReadWritePart
{
  Q_OBJECT

  private:
    void             makeMenus();
    QString          _hastodo( Task* task, const QString &taskname ) const;

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
    KAction*         actionPreferences;
    KAction*         actionClipTotals;
    KAction*         actionClipHistory;

    friend class KarmTray;

public:
    karmPart(QWidget *parentWidget, const char *widgetName,
             QObject *parent, const char *name);

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

};

class KInstance;
class KAboutData;

class karmPartFactory : public KParts::Factory
{
    Q_OBJECT
public:
    karmPartFactory();
    virtual ~karmPartFactory();
    virtual KParts::Part* createPartObject( QWidget *parentWidget, const char *widgetName,
                                            QObject *parent, const char *name,
                                            const char *classname, const QStringList &args );
    static KInstance* instance();
 
private:
    static KInstance* s_instance;
    static KAboutData* s_about;
};

#endif // _KARMPART_H_
