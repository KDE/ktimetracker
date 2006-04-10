#include <stdlib.h>             // abs()

#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qstring.h>
#include <qvalidator.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QHBoxLayout>
#include <QKeyEvent>

#include <klocale.h>            // i18n
#include "ktimewidget.h"

enum ValidatorType { HOUR, MINUTE };

class TimeValidator : public QValidator
{
  public:
    TimeValidator( ValidatorType tp, QWidget *parent=0, const char *name=0)
      : QValidator(parent, name)
    {
      _tp = tp;
    }
    State validate(QString &str, int &) const
    {
      if (str.isEmpty())
        return Acceptable;

      bool ok;
      int val = str.toInt( &ok );
      if ( ! ok )
        return Invalid;

      if ( str.contains('-') != 0 )
        return Invalid;

      if ( _tp==MINUTE && val >= 60  )
        return Invalid;
      else
        return Acceptable;
    }

  public:
    ValidatorType _tp;
};


class KarmLineEdit : public QLineEdit
{

  public:
    KarmLineEdit( QWidget* parent, const char* name = 0 )
      : QLineEdit( parent, name ) {}

protected:

  virtual void keyPressEvent( QKeyEvent *event )
  {
    QLineEdit::keyPressEvent( event );
    if ( text().length() == 2 && !event->text().isEmpty() )
      focusNextPrevChild(true);
  }
};


KArmTimeWidget::KArmTimeWidget( QWidget* parent, const char* name )
  : QWidget(parent, name)
{
  QHBoxLayout *layout = new QHBoxLayout(this);

  _hourLE = new QLineEdit( this);
  // 9999 hours > 1 year!
  // 999 hours = 41 days  (That should be enough ...)
#warning "Qt4: fixme" 
  _hourLE->setFixedWidth( fontMetrics().maxWidth() * 3
                          + 2 /** _hourLE->frameWidth()*/ + 2);
  layout->addWidget(_hourLE);
  TimeValidator *validator = new TimeValidator( HOUR, _hourLE,
                                                "Validator for _hourLE");
  _hourLE->setValidator( validator );
  _hourLE->setAlignment( Qt::AlignRight );


  QLabel *hr = new QLabel( i18nc( "abbreviation for hours", " hr. " ), this );
  layout->addWidget( hr );

  _minuteLE = new KarmLineEdit(this);

  // Minutes lineedit: Make room for 2 digits
#warning "Qt4: fixme"
  _minuteLE->setFixedWidth( fontMetrics().maxWidth() * 2
                            + 2 /** _minuteLE->frameWidth()*/ + 2);
  layout->addWidget(_minuteLE);
  validator = new TimeValidator( MINUTE, _minuteLE, "Validator for _minuteLE");
  _minuteLE->setValidator( validator );
  _minuteLE->setMaxLength(2);
  _minuteLE->setAlignment( Qt::AlignRight );

  QLabel *min = new QLabel( i18nc( "abbreviation for minutes", " min. " ), this );
  layout->addWidget( min );

  layout->addStretch(1);
  setFocusProxy( _hourLE );
}

void KArmTimeWidget::setTime( int hour, int minute )
{
  QString dummy;

  dummy.setNum( hour );
  _hourLE->setText( dummy );

  dummy.setNum( abs(minute) );
  if (abs(minute) < 10 ) {
    dummy = QString::fromLatin1( "0" ) + dummy;
  }
  _minuteLE->setText( dummy );
}

long KArmTimeWidget::time() const
{
  bool ok;
  int h, m;

  h = _hourLE->text().toInt( &ok );
  m = _minuteLE->text().toInt( &ok );

  // if h is negative, we have to *subtract* m
  return h * 60 + ( ( h < 0) ? -1 : 1 ) * m;
}
