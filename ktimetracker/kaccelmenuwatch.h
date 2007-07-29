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
#ifndef KARM_K_ACCEL_MENU_WATCH_H
#define KARM_K_ACCEL_MENU_WATCH_H

#include <QObject>
#include <QList>

#include <KStandardShortcut>

class QMenu;

#ifdef __GNUC__
#warning Port me to QAction
#endif
class KAccel;

/**
 * Easy updating of menu accels when changing a KAccel object. 
 *
 * Since a KAccel object does not keep track of menu items to which it
 * is connected, we normally have to manually call 
 * KAccel::changeMenuAccel() again for each update of the KAccel object.
 *
 * With KAccelMenuWatch you provide the kaccel object and the menu
 * items to which it connects, and if you update the kaccel you just have
 * to call KAccelMenuWatch::updateMenus() and the menu items will be updated.
 *
 * It is safe to delete menus that have connections handled by this class.
 * On deletion of a menu, all associated accelerators will be deleted.
 *
 * Note that you _have_ to call KAccelMenuWatch::updateMenus() after you
 * connect the * accelerators, as they are not activated till then.
 * 
 * @author Sirtaj Singh Kang (taj@kde.org)
 */

class KAccelMenuWatch : public QObject
{
  Q_OBJECT

  private:
    enum AccelType { StdAccel, StringAccel };

    struct AccelItem {
      QMenu  *menu;
      int itemId;

      AccelType type;

      // only one of these is used at a time
      QString action;
      KStandardShortcut::StandardShortcut stdAction;
    };

    KAccel *_accel;
    QList<AccelItem*> _accList;
    QList<QMenu*> _menuList;

    QMenu  *_menu;

    KAccelMenuWatch::AccelItem *newAccelItem( QMenu *menu, 
                                              int itemId, AccelType type );

  public:
    /**
     * KAccelMenuWatch Constructor
     */
    KAccelMenuWatch( KAccel *accel, QObject *parent = 0 );

    /**
     * KAccelMenuWatch Destructor
     */
    virtual ~KAccelMenuWatch() {}

    /** 
     * Set the menu on which connectAccel calls will operate.
     * All subsequent calls to connectAccel will be associated
     * with this menu. You can call this function any number of
     * times, so multiple menus can be handled.
     */
    void setMenu( QMenu *menu );

    /** 
     * Return the last menu set with KAccelMenuWatch::setMenu(QPopupMenu*),
     * or 0 if none has been set.
     */
    QMenu *currentMenu() const  { return _menu; }

    /** 
     * Connect the menu item identified to currentMenu()/id to
     * the accelerator action.
     */
    void connectAccel( int itemId, const char *action );

    /** 
     * Same as above, but connects to standard accelerators.
     */
    void connectAccel( int itemId, KStandardShortcut::StandardShortcut );

  public Q_SLOTS:
    /** 
     * Updates all menu accelerators. Call this after all accelerators
     * have been connected or the kaccel object has been updated.
     */
    void updateMenus();

  private Q_SLOTS:
    void removeDeadMenu();

  private:
    KAccelMenuWatch& operator=( const KAccelMenuWatch& );
    KAccelMenuWatch( const KAccelMenuWatch& );
};

#endif // KARM_K_ACCEL_MENU_WATCH_H
