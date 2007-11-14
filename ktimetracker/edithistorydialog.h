/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

#ifndef KTIMETRACKER_EDIT_HISTORY_DIALOG_H
#define KTIMETRACKER_EDIT_HISTORY_DIALOG_H

#include <KDialog>

class TaskView;
class QTableWidget;

class EditHistoryDialog : public KDialog 
{
  Q_OBJECT

public:
  explicit EditHistoryDialog( TaskView *parent );
  QTableWidget* tableWidget();

private:
  QTableWidget *mHistoryWidget;
  TaskView *mParent;
  /** 
   * This lists all events in the calendar.
   * This procedure is triggered on user request.
   */
  void listAllEvents();

private Q_SLOTS:
  /**
   * The historywidget contains all events and can be shown with File | Edit History. 
   * The user can change dates and comments in there.
   * A change triggers this procedure, it shall store the new values in the calendar.
   */
  void historyWidgetCellChanged( int row, int col );
};

#endif
