/*
 *     Copyright (C) 2000 by Jesper Pedersen <blackie@kde.org>
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

#ifndef KARM_PREFERENCES_H
#define KARM_PREFERENCES_H

#include <KPageDialog>


/**
  Provide an interface to the configuration options for the program.
 */
class Preferences : public KPageDialog
{
  Q_OBJECT

  public:
    static Preferences *instance();

    bool readBoolEntry( const QString& uid );
    void writeEntry( const QString &key, bool value );
    void deleteEntry( const QString &key );

  private:
    Preferences();
    static Preferences *mInstance;
};

#endif // KARM_PREFERENCES_H

