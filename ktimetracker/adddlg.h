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

class TimeValidator :public QValidator 
{
public:
	TimeValidator(QWidget *parent=0, const char *name=0) :QValidator(parent, name) {}
	virtual State validate(QString &, int &) const;
	bool extractTime(QString str, long *res) const;
};

class AddTaskDialog : public KDialogBase
{
  Q_OBJECT

  public:
    AddTaskDialog(QString caption);
    void setTask(const QString &name, long time, long sessionTime );
    QString taskName( void ) const;
    long totalTime( void ) const; 
    long sessionTime( void ) const; 

  signals:
    /** 
     * raised on click of OK or Cancel.
     * true if Ok clicked, false if Cancel clicked.
     */
    void finished( bool );

  private:
  	QLineEdit *_name;
	  QLineEdit *_totalTime;
	  QLineEdit *_sessionTime;
	  TimeValidator *_totalValidator;
	  TimeValidator *_sessionValidator;
};





#endif // KarmAddDlg_included

