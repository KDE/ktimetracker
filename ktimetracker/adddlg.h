/*
 *   karm
 *   This file only: Copyright (C) 1999  Espen Sand, espensa@online.no
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef KarmAddDlg_included
#define KarmAddDlg_included

#include <stdlib.h>
#include <kdialogbase.h>
#include <qvalidator.h>
#include "adddlg.h"
class QLineEdit;
class KTimeWidget;
class QRadioButton;

class AddTaskDialog : public KDialogBase
{
  Q_OBJECT

  public:
    AddTaskDialog(QString caption, bool editDlg);
    void setTask(const QString &name, long time, long sessionTime );
    QString taskName( void ) const;
	  void status( long *total, long *totalDiff, long *session, long *sessionDiff ) const;
	
  private slots:
	void slotAbsolutePressed();
	void slotRelativePressed();

  private:
  	QLineEdit* _name;
	  KTimeWidget* _totalTW;
	  KTimeWidget* _sessionTW;
	  KTimeWidget* _diffTW;
	  QComboBox* _operator;

	  long origTotal;
  	long origSession;

	  QRadioButton *_absoluteRB;
	  QRadioButton *_relativeRB;

  	QLabel* _totalLA;
	  QLabel* _sessionLA;
};





#endif // KarmAddDlg_included

