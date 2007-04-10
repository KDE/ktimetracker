/*
* kaccelmenuwatch.cpp -- Implementation of class KAccelMenuWatch.
* Author:    Sirtaj Singh Kang
* Generated: Thu Jan  7 15:05:26 EST 1999
*
*
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

#include <assert.h>
#include <kdebug.h>
#include <QMenu>

#include "kaccelmenuwatch.h"

KAccelMenuWatch::KAccelMenuWatch( KAccel *accel, QObject *parent )
  : QObject( parent ),
  _accel( accel ),
  _menu ( 0 )
{
  _accList.setAutoDelete( true );
  _menuList.setAutoDelete( false );
}

void KAccelMenuWatch::setMenu( QMenu *menu )
{
  assert( menu );

  // we use  _menuList to ensure that the signal is
  // connected only once per menu.

  if ( !_menuList.findRef( menu ) ) {
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

  if ( !_menuList.findRef( sdr ) )
    return;

  // remove all accels

  AccelItem *accel;
  for ( accel = _accList.first(); accel; accel = _accList.next() )
  {
loop:
    if( accel && accel->menu == sdr ) {
      _accList.remove();
      accel = _accList.current();
      goto loop;
    }
  }

  // remove from menu list
  _menuList.remove( sdr );

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
