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
#include <kdateedit.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kurlrequester.h>
#include <q3buttongroup.h>
#include <QComboBox>
#include <QRadioButton>

#include "csvexportdialog.h"
#include "reportcriteria.h"

CSVExportDialog::CSVExportDialog( ReportCriteria::REPORTTYPE rt,
                                  QWidget *parent 
                                  ) 
  : CSVExportDialogBase( parent )
{
  connect(btnExportClip, SIGNAL(clicked()), this, SLOT(exPortToClipBoard()));
  connect(btnExport, SIGNAL(clicked()), this, SLOT(exPortToCSVFile()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(btnExport, SIGNAL(clicked()), this, SLOT(accept()));
  connect(urlExportTo,SIGNAL(textChanged(QString)), this, SLOT(enableExportButton()));
  switch ( rt ) {
    case ReportCriteria::CSVTotalsExport:
      grpDateRange->setEnabled( false );
      grpDateRange->hide();
      rc.reportType = rt;
      break;
    case ReportCriteria::CSVHistoryExport:
      grpDateRange->setEnabled( true );
      rc.reportType = rt;
      break;
    default:
      break;

  }

  // If decimal symbol is a comma, then default field seperator to semi-colon.
  // In France and Germany, one-and-a-half is written as 1,5 not 1.5
  QString d = KGlobal::locale()->decimalSymbol();
  if ( "," == d ) CSVExportDialogBase::radioSemicolon->setChecked(true);
  else CSVExportDialogBase::radioComma->setChecked(true);

}

void CSVExportDialog::enableExportButton()
{
  btnExport->setEnabled( !urlExportTo->lineEdit()->text().isEmpty() );
}

void CSVExportDialog::enableTasksToExportQuestion()
{
  return;
  //grpTasksToExport->setEnabled( true );      
}

void CSVExportDialog::exPortToClipBoard()
{
  rc.bExPortToClipBoard=true;
  accept();
}

void CSVExportDialog::exPortToCSVFile()
{
  rc.bExPortToClipBoard=false;
  accept();
}

ReportCriteria CSVExportDialog::reportCriteria()
{
  rc.url = urlExportTo->url();
  rc.from = dtFrom->date();
  rc.to = dtTo->date();
  rc.decimalMinutes = (  combodecimalminutes->currentText() == i18n( "Decimal" ) );
  kDebug(5970) << "rc.decimalMinutes is " << rc.decimalMinutes << endl;

  if ( radioComma->isChecked() )          rc.delimiter = ",";
  else if ( radioTab->isChecked() )       rc.delimiter = "\t";
  else if ( radioSemicolon->isChecked() ) rc.delimiter = ";";
  else if ( radioSpace->isChecked() )     rc.delimiter = " ";
  else if ( radioOther->isChecked() )     rc.delimiter = txtOther->text();
  else {
    kDebug(5970) 
        << "*** CSVExportDialog::reportCriteria: Unexpected delimiter choice '"
        << endl;
    rc.delimiter = "\t";
  }

  rc.quote = cboQuote->currentText();
  rc.sessionTimes = (i18n("Session Times") == combosessiontimes->currentText());
  rc.allTasks = (i18n("All Tasks") == comboalltasks->currentText());

  return rc;
}

#include "csvexportdialog.moc"
