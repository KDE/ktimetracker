/**********************************************************************

	--- Dlgedit generated file ---

	File: adddlg.h
	Last generated: Fri Jun 20 07:29:11 1997

 *********************************************************************/

#ifndef KarmAddDlg_included
#define KarmAddDlg_included

#include<stdlib.h>

#include "adddata.h"

///
class KarmAddDlg : public KAddDlgData
{
    Q_OBJECT

public:

    KarmAddDlg
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~KarmAddDlg();

	///
	void setTask(const char *name, long time);

	///
	QString taskName() const
		{ return _taskName->text(); };
	///
	long taskTime() const
		{ return atol( _taskTime->text().ascii()); };

signals:
	/** raised on click of OK or Cancel.
	* TRUE if Ok clicked, FALSE if Cancel clicked.
	*/
	void finished( bool );

protected slots:
	///
	void cancelClicked()
		{ emit finished( FALSE ); };
	///
	void okClicked()
		{ emit finished( TRUE ); };

};

#endif // KarmAddDlg_included
