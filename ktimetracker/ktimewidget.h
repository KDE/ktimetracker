
#ifndef KTIMEWIDGET_H_
#define KTIMEWIDGET_H_

class QLineEdit;
class KarmLineEdit;

class KTimeWidget :public QWidget 
{
public:
	KTimeWidget( QWidget* parent = 0, const char* name = 0 );
	void setTime( int hour, int minute );
	long time() const;

private:
	QLineEdit *_hourLE;
	KarmLineEdit *_minuteLE;
};

#endif
