#include "top.h"

#include <qtooltip.h>
#include <kapp.h>

#include "docking.h"
#include <klocale.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kpopupmenu.h>

DockWidget::DockWidget( KarmWindow* parent, const char *name)
  : KDockWindow( parent, name ) 
{
  setPixmap( SmallIcon(QString::fromLatin1("karm")));
}

DockWidget::~DockWidget() {
}


#include "docking.moc"






