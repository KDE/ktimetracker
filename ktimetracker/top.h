#ifndef KARM_TOP_H
#define KARM_TOP_H

#include <ktmainwindow.h>

class QPopupMenu;
class Karm;
class KAccel;
class KAccelMenuWatch;

/// The top level widget for Karm.
class KarmWindow : public KTMainWindow
{
  Q_OBJECT

  private:
    QPopupMenu	*_fileMenu;
    QPopupMenu	*_clockMenu;
    QPopupMenu	*_taskMenu;
    KAccel		*_accel;
    KAccelMenuWatch	*_watcher;
    Karm		*_karm;
    long		_totalTime;
    char		*_sessionTimeBuffer;

  public:
    KarmWindow();
    virtual ~KarmWindow();

  protected slots:
    void prefs();
    void resetSessionTime(); 
    void updateTime();
    void clockStartMsg();
    void clockStopMsg();

  protected:
    virtual void saveProperties( KConfig* );
    //	virtual void readSettings( KConfig * );
    //	virtual void writeSettings( KConfig * );

  private:
    void initAccelItems();
    void connectAccels();
    void makeMenus();
};

#endif


