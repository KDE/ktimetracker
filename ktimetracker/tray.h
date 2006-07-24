#ifndef KARM_TRAY_H
#define KARM_TRAY_H

#include <q3ptrvector.h>
#include <QPixmap>
#include <q3ptrlist.h>
//Added by qt3to4:
#include <Q3PopupMenu>
// experiement
// #include <kmenu.h>
#include <ksystemtrayicon.h>

#include "task.h"
#include "karm_part.h"

class KarmPart;

class Q3PopupMenu;
class QTimer;

class MainWindow;
// experiment
// class KMenu;

class KarmTray : public KSystemTrayIcon
{
  Q_OBJECT

  public:
    KarmTray(MainWindow * parent);
    KarmTray(karmPart * parent);
    KarmTray();
    ~KarmTray();

  private:
    int _activeIcon;
    static Q3PtrVector<QPixmap> *icons;
    QTimer *_taskActiveTimer;

  public slots:
    void startClock();
    void stopClock();
    void resetClock();
    void updateToolTip( Q3PtrList<Task> activeTasks);
    void initToolTip();

  protected slots:
    void advanceClock();
    
  // experiment
  /*
    void insertTitle(QString title);

  private:
    KMenu *trayPopupMenu;
    QPopupMenu *trayPopupMenu2;
    */
};

#endif // KARM_TRAY_H
