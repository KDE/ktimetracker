#ifndef KARM_K_TIME_WIDGET_H
#define KARM_K_TIME_WIDGET_H

class QLineEdit;
class QWidget;

class KarmLineEdit;

/**
 * Widget used for entering minutes and seconds with validation.
 */

class KArmTimeWidget : public QWidget 
{
  public:
    KArmTimeWidget( QWidget* parent = 0, const char* name = 0 );
    void setTime( int hour, int minute );
    long time() const;

  private:
    QLineEdit *_hourLE;
    KarmLineEdit *_minuteLE;
};

#endif // KARM_K_TIME_WIDGET_H
