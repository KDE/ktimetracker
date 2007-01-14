#ifndef KARM_UTILITY_H
#define KARM_UTILITY_H

#include <stdlib.h>

#include <kglobal.h>
#include <klocale.h>
#include "karmutility.h"

QString formatTime( long minutes, bool decimal )
{
  QString time;
  if ( decimal ) {
    time.sprintf("%.2f", minutes / 60.0);
    time.replace( '.', KGlobal::locale()->decimalSymbol() );
  }
  else time.sprintf("%ld:%02ld", minutes / 60, labs(minutes % 60));
  return time;
}

#endif // KARM_UTILITY_H
