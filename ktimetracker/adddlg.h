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
class QLineEdit;


class AddTaskDialog : public KDialogBase
{
  Q_OBJECT

  public:
    AddTaskDialog( QWidget *parent=0, const char *name=0, bool modal=true );
    void setTask(const QString &name, long time );
    QString taskName( void ) const;
    long taskTime( void ) const; 

  protected slots:
    virtual void slotCancel( void );
    virtual void slotOk( void );

  signals:
    /** 
     * raised on click of OK or Cancel.
     * true if Ok clicked, false if Cancel clicked.
     */
    void finished( bool );

  private:
    QLineEdit *mTaskName;
    QLineEdit *mTaskTime;
};





#endif // KarmAddDlg_included

