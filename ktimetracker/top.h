#ifndef KARM_TOP_H
#define KARM_TOP_H

#include <ktmainwindow.h>
#include <kkeydialog.h>

class QPopupMenu;
class Karm;
class KAccel;
class KAccelMenuWatch;
class Preferences;
class QTimer;

class KarmWindow : public KTMainWindow
{
  Q_OBJECT

private:
  KAccel		*_accel;
  KAccelMenuWatch	*_watcher;
  Karm		*_karm;
  long		_totalTime;
  Preferences *_preferences;

  public:
    KarmWindow();
    virtual ~KarmWindow();

  public slots:
	void updateTime( long difference );

protected slots:
  void keyBindings();
  void resetSessionTime(); 
  void updateTime();
  void updateStatusBar();
  void save();
  void quit();
  void print();

protected:
  virtual void saveProperties( KConfig* );
  void saveGeometry();
  void loadGeometry();

  private:
    void makeMenus();
    KDialogBase *dialog;
};

#endif


