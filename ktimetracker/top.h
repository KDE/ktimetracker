#ifndef KARM_TOP_H
#define KARM_TOP_H

#include<ktopwidget.h>

class QPopupMenu;
class KMenuBar;
class KToolBar;
class KStatusBar;
class Karm;

/// The top level widget for Karm.
class KarmWindow : public KTopLevelWidget
{
	Q_OBJECT;
private:
	KMenuBar	*_mainMenu;

	QPopupMenu	*_fileMenu;
	QPopupMenu	*_clockMenu;
	QPopupMenu	*_taskMenu;
	QPopupMenu	*_helpMenu;

	KToolBar	*_toolBar;
	KStatusBar	*_statusBar;
	Karm		*_karm;

	long		_totalTime;
	char		*_sessionTimeBuffer;

public:
	///
	KarmWindow();
	///
	virtual ~KarmWindow();

public slots:

	///
	void help();
	///
	void about();

protected slots:
	///
	void updateTime();
	///
	void clockStartMsg();
	///
	void clockStopMsg();

};

#endif
