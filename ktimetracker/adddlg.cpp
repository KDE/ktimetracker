/**********************************************************************

	--- Dlgedit generated file ---

	File: adddlg.cpp
	Last generated: Fri Jun 20 07:29:11 1997

 *********************************************************************/

#include "adddlg.h"
#include "adddata.h"

#define Inherited KAddDlgData

KarmAddDlg::KarmAddDlg
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	// This is just a kind of default (Caption is reset in class Karm)
	setCaption( "KArm: New Task" );
}


KarmAddDlg::~KarmAddDlg()
{
}

void KarmAddDlg::setTask( const char *name, long time )
{
	QString timeStr;

	_taskName->setText( name );

	_taskTime->setText( timeStr.setNum( time ) );
}
