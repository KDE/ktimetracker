#ifndef _DOCKING_H_
#define _DOCKING_H_

#include <kdockwindow.h>

class KarmWindow;

class DockWidget : public KDockWindow {

  Q_OBJECT

public:
  DockWidget( KarmWindow* parent, const char *name=0);
  ~DockWidget();

};

#endif



