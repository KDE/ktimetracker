#ifndef KARM_TOP_H
#define KARM_TOP_H

#include<ktopwidget.h>

class QPopupMenu;
class KMenuBar;
class KToolBar;
class KStatusBar;
class Karm;
class KAccel;
class KAccelMenuWatch;

/// The top level widget for Karm.
class KarmWindow : public KTMainWindow
{
	Q_OBJECT
private:
	KMenuBar	*_mainMenu;

	QPopupMenu	*_fileMenu;
	QPopupMenu	*_clockMenu;
	QPopupMenu	*_taskMenu;
	QPopupMenu	*_helpMenu;

	KToolBar	*_toolBar;
	KStatusBar	*_statusBar;
	KAccel		*_accel;
	KAccelMenuWatch	*_watcher;

	Karm		*_karm;

	long		_totalTime;
	char		*_sessionTimeBuffer;

public:
	KarmWindow();
	virtual ~KarmWindow();

protected slots:
	void prefs();
	void updateTime();
	void clockStartMsg();
	void clockStopMsg();

protected:
	virtual void saveProperties( KConfig* );

//	virtual void readSettings( KConfig * );
//	virtual void writeSettings( KConfig * );

private:
	void initAccelItems();
	void connectAccels();
	void makeMenus();
};

#endif
