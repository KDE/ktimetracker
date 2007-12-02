/*
 *     Copyright (C) 2000 by Jesper Perderson <blackie@kde.org>
 *                   2007 the ktimetracker developers
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
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

#include "ktimewidget.h"

#include <stdlib.h>             // abs()

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QString>
#include <QValidator>
#include <QWidget>
#include <kdebug.h>
#include <KLineEdit>
#include <KLocale>

enum ValidatorType { HOUR, MINUTE };

class TimeValidator : public QValidator
{
  public:
    TimeValidator( ValidatorType tp, QWidget *parent=0, const char *name=0)
      : QValidator(parent)
    {
      if ( name ) { setObjectName( name ); }
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
      else
        return Acceptable;
    }

  public:
    ValidatorType _tp;
};


class KarmLineEdit : public KLineEdit
{

  public:
    KarmLineEdit( QWidget* parent, const char* name = 0 )
      : KLineEdit( parent ) { setObjectName( name ); }

protected:

  virtual void keyPressEvent( QKeyEvent *event )
  {
    KLineEdit::keyPressEvent( event );
    if ( text().length() == 2 && !event->text().isEmpty() )
      focusNextPrevChild(true);
  }
};


KArmTimeWidget::KArmTimeWidget( QWidget* parent, const char* name )
  : QWidget(parent)
{
  setObjectName( name );
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setMargin( 0 );
  layout->setSpacing( 0 );

  _hourLE = new KLineEdit( this);
  // 9999 hours > 1 year!
  // 999 hours = 41 days  (That should be enough ...)

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
  kDebug(5970) << "Entering KArmTimeWidget::setTime( "<< hour << ", " << minute << " )";
  QString dummy;

  dummy.setNum( hour + (minute/60));
  _hourLE->setText( dummy );

  dummy.setNum( abs(minute%60) );
  if (abs(minute) < 10 ) 
  {
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
