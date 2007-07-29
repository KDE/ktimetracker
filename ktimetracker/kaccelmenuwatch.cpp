/*
*     Copyright (C) 1999 by Sirtaj Singh Kang (taj@kde.org)
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

#include "kaccelmenuwatch.h"

#include <cassert>

#include <QMenu>

#include <KDebug>

KAccelMenuWatch::KAccelMenuWatch( KAccel *accel, QObject *parent )
  : QObject( parent ),
  _accel( accel ),
  _menu ( 0 )
{
}

void KAccelMenuWatch::setMenu( QMenu *menu )
{
  assert( menu );

  // we use  _menuList to ensure that the signal is
  // connected only once per menu.

  if ( _menuList.indexOf( menu ) == -1 ) {
    _menuList.append( menu );
    connect( menu, SIGNAL(destroyed()), this, SLOT(removeDeadMenu()) );
  }

  _menu = menu;
}

void KAccelMenuWatch::connectAccel( int itemId, const char *action )
{
  AccelItem *item = newAccelItem( _menu, itemId, StringAccel ) ;
  item->action  = QString::fromLocal8Bit( action );
}

void KAccelMenuWatch::connectAccel( int itemId, KStandardShortcut::StandardShortcut accel )
{
  AccelItem *item = newAccelItem( _menu, itemId, StdAccel ) ;
  item->stdAction  = accel;
}

void KAccelMenuWatch::updateMenus()
{
  kDebug(5970) << "This is KAccelMenuWatch::updateMenus" << endl;
/*
  assert( _accel != 0 );

  Q3PtrListIterator<AccelItem> iter( _accList );
  AccelItem *item;

  for( ; (item = iter.current()) ; ++iter ) {
    // These setAccel calls were converted from all changeMenuAccel calls
    // as descibed in KDE3PORTING.html
    switch( item->type ) {
      case StringAccel:
#ifdef __GNUC__
#warning Port me!
#endif
//        item->menu->setAccel( _accel->shortcut( item->action ).keyQt(), item->itemId );
        break;
      case StdAccel:
#ifdef __GNUC__
#warning Port me!
#endif
//        item->menu->setAccel( KStandardShortcut::shortcut( item->stdAction ).keyQt(), item->itemId );
        break;
      default:
        break;
    }
  }
*/
}

void KAccelMenuWatch::removeDeadMenu()
{
  QMenu *sdr = (QMenu *) sender();
  assert( sdr );

  if ( _menuList.indexOf( sdr ) == -1 )
    return;

  // remove all accels
  foreach (AccelItem *accel, _accList) {
    if ( accel && accel->menu == sdr ) {
      _accList.removeAll( accel );
      delete accel; // since setAutoDelete is not available in QList
    }
  }

  // remove from menu list
  _menuList.removeAll( sdr );

  return;
}

KAccelMenuWatch::AccelItem *KAccelMenuWatch::newAccelItem( QMenu *,
    int itemId, AccelType type )
{
  AccelItem *item = new AccelItem;

  item->menu  = _menu;
  item->itemId  = itemId;
  item->type  = type;

  _accList.append( item );

  return item;
}

#include "kaccelmenuwatch.moc"
