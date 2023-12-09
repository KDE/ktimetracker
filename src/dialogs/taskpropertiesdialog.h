/*
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KTIMETRACKER_TASKPROPERTIESDIALOG_H
#define KTIMETRACKER_TASKPROPERTIESDIALOG_H

#include <QDialog>
#include <QList>
#include <QScrollArea>


#include "desktoplist.h"

class QLineEdit;
class QPlainTextEdit;
class QGroupBox;
class QCheckBox;

class TaskPropertiesDialog : public QDialog
{
public:
    TaskPropertiesDialog(QWidget *parent,
                         const QString &caption,
                         const QString &name,
                         const QString &description,
                         const DesktopList &desktops);
    ~TaskPropertiesDialog() override = default;

    QString name() const;
    QString description() const;
    DesktopList desktops() const;

private:
    QScrollArea *m_scrollArea;
    int m_numDesktops;
    QLineEdit *m_nameText;
    QPlainTextEdit *m_descText;
    QGroupBox *m_trackingGroup;
    QList<QCheckBox *> m_trackingDesktops;
};

#endif // KTIMETRACKER_TASKPROPERTIESDIALOG_H
