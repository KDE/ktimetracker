/*
 *     Copyright (C) 2005-2008 by Thorsten Staerk <kde@staerk.de>
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

#ifndef _KTIMETRACKERPART_H_
#define _KTIMETRACKERPART_H_

#include <KParts/ReadWritePart>

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
class KTimeTrackerPart : public KParts::ReadWritePart
{
public:
    KTimeTrackerPart(QWidget *parentWidget, QObject *parent, const QVariantList&);

    ~KTimeTrackerPart() override = default;

    /**
     * openFile() opens the icalendar file that contains the tasks and events.
     * It has been inherited from KParts::ReadWritePart where it was protected
     * openFile() just calls openFile(<standard ical file>).
     */
    bool openFile() override;
    virtual bool openFile(const QString& path);

protected:
    bool saveFile() override;

public Q_SLOTS:
   void setStatusBar(const QString& qs);
   void keyBindings();

private:
    void makeMenus();

    TimetrackerWidget *m_mainWidget;
};

#endif // _KTIMETRACKERPART_H_
