/*
 *     Copyright (C) 2009 by Thorsten Staerk and Mathias Soeken <msoeken@tzi.de>
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

#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include <QtGui/QDialog>
#include "taskview.h"

namespace Ui
{
    class historydialog;
}

class historydialog : public QDialog
{
    Q_OBJECT
public:
    historydialog(TaskView *parent);
    ~historydialog();
    QString listallevents();
    QString refresh();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::historydialog *m_ui;
    TaskView *mparent;

private Q_SLOTS:
  /**
   * The historywidget contains all events and can be shown with File | Edit History.
   * The user can change dates and comments in there.
   * A change triggers this procedure, it shall store the new values in the calendar.
   */
  void on_deletepushbutton_clicked();
  void historyWidgetCellChanged( int row, int col );
  void on_okpushbutton_clicked();
};

#endif // HISTORYDIALOG_H
