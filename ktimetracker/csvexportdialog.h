/*
 *   Copyright (C) 2004  Mark Bucciarelli <mark@hubcapconsulting.com>
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
#ifndef CSVEXPORTDIALOG_H
#define CSVEXPORTDIALOG_H

#include "csvexportdialog_base.h"
#include "reportcriteria.h"

class CSVExportDialog : public CSVExportDialogBase
{
  Q_OBJECT

  public:
    CSVExportDialog( ReportCriteria::REPORTTYPE rt,
                     QWidget *parent = 0, 
                     const char *name = 0
                     );
    
    /**
     Enable the "Tasks to export" question in the dialog.

     Since Karm does not have the concept of a single root task, when the user
     requests a report on a top-level task, it is impossible to know if they
     want all tasks or just the currently selected top-level task.

     Stubbed for 3.3 release as CSV export of totals doesn't suppor this option.
     */
    void enableTasksToExportQuestion();

    /**
     Return an object that encapsulates the choices the user has made.
     */
    ReportCriteria reportCriteria();

  private slots:

    /**
    Enable export button if export url entered.
    */
    void enableExportButton();

  private:
    ReportCriteria rc;
};

#endif
