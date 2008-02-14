/*
 *     Copyright (C) 2005 by Thorsten Staerk <kde@staerk.de>
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

#ifndef _KARMPART_H_
#define _KARMPART_H_

#include <kparts/part.h>
#include "karmerrors.h"
#include <kparts/factory.h>
#include "reportcriteria.h"

class TrayIcon;
class QWidget;
class TimetrackerWidget;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Thorsten Staerk (kde at staerk dot de)
 * @version 0.1
 */
class karmPart : public KParts::ReadWritePart
{
  Q_OBJECT

  private:
    void               makeMenus();
    TrayIcon          *mTray;
    TimetrackerWidget *mMainWidget;


    friend class TrayIcon;

public:
    karmPart(QWidget *parentWidget, QObject *parent, const QStringList&);

    void taskViewCustomContextMenuRequested( const QPoint& );
    TimetrackerWidget* MainWidget() { return mMainWidget; };

    virtual ~karmPart();
    static KAboutData *createAboutData();
protected:
    virtual bool saveFile();
    virtual bool openFile();

public Q_SLOTS:
   void setStatusBar(const QString & qs);

};

#endif // _KARMPART_H_
