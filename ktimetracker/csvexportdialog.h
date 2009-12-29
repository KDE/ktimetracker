/*
 *     Copyright (C) 2004 by Mark Bucciarelli <mark@hubcapconsulting.com>
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

#ifndef CSVEXPORTDIALOG_H
#define CSVEXPORTDIALOG_H

#include "ui_csvexportdialog_base.h"
#include "reportcriteria.h"

class CSVExportDialogBase : public KDialog, public Ui::CSVExportDialogBase
{
public:
  CSVExportDialogBase( QWidget *parent ) : KDialog( parent ) {
    setupUi( this );

    setMainWidget( page );
    setButtons( KDialog::User1 | KDialog::Ok | KDialog::Cancel );
    setButtonText( KDialog::Ok, i18nc("@action:button", "&Export") );
    setButtonText( KDialog::User1, i18nc("@action:button", "E&xport to Clipboard") );
    setButtonIcon( KDialog::User1, KIcon( "klipper" ) );
    enableButton( KDialog::Ok, false );
  }
};



class CSVExportDialog : public CSVExportDialogBase
{
  Q_OBJECT

  public Q_SLOTS:
    void exPortToClipBoard();
    void exPortToCSVFile();

  public:
    explicit CSVExportDialog( ReportCriteria::REPORTTYPE rt,
                     QWidget *parent = 0 
                     );

    /**
     Enable the "Tasks to export" question in the dialog.

     Since ktimetracker does not have the concept of a single root task, when the user
     requests a report on a top-level task, it is impossible to know if they
     want all tasks or just the currently selected top-level task.

     Stubbed for 3.3 release as CSV export of totals doesn't support this option.
     */
    void enableTasksToExportQuestion();

    /**
     Return an object that encapsulates the choices the user has made.
     */
    ReportCriteria reportCriteria();

  private Q_SLOTS:

    /**
    Enable export button if export url entered.
    */
    void enableExportButton();

  private:
    ReportCriteria rc;
};

#endif
