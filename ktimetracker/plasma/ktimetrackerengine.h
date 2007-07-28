/*
 *   Copyright (C) 2007 Mathias Soeken <msoeken@tzi.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KTIMETRACKERENGINE_H
#define KTIMETRACKERENGINE_H

#include <plasma/dataengine.h>

class QTimer;

class KTimetrackerEngine : public Plasma::DataEngine
{
Q_OBJECT

public:
  KTimetrackerEngine( QObject* parent, const QStringList& args );
  ~KTimetrackerEngine() {}

protected:
  bool sourceRequested( const QString &name );

private:
  QTimer *m_timer;

private Q_SLOTS:
  void updateData();
};

K_EXPORT_PLASMA_DATAENGINE(ktimetracker, KTimetrackerEngine)

#endif
