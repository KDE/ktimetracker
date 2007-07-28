#include "ktimetrackerapplet.h"

#include <math.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <KLocale>

#include <plasma/svg.h>
#include <plasma/widgets/lineedit.h>

KTimetrackerApplet::KTimetrackerApplet(QObject *parent, const QStringList &args)
    : Plasma::Applet(parent, args)
{
  m_pixelSize = 250;
  m_theme = new Plasma::Svg("widgets/background", this);
  m_theme->setContentType(Plasma::Svg::SingleImage);
  m_theme->resize(m_pixelSize, m_pixelSize);

  m_label = new Plasma::LineEdit( this );
  m_label->setDefaultText( "ktimetracker is not started." );
  m_label->setTextInteractionFlags( Qt::NoTextInteraction );
  m_label->setPos( 27, 27 );

  Plasma::DataEngine *engine = dataEngine( "ktimetracker" );
  engine->connectSource( "timetracker", this );
  updated( "timetracker", engine->query( "timetracker" ) );
  constraintsUpdated();
}

QSizeF KTimetrackerApplet::contentSize() const
{
  return m_size;
}

void KTimetrackerApplet::constraintsUpdated()
{
  m_size = m_theme->size();
  update();
}

void KTimetrackerApplet::updated(const QString& source, const Plasma::DataEngine::Data &data)
{
  Q_UNUSED(source);
  bool started = data["ktimetracker-running"].toBool();
  if ( started ) {
    m_label->setDefaultText( "ktimetracker is started." );
  } else {
    m_label->setDefaultText( "ktimetracker is not started." );
  }
  update();
}

KTimetrackerApplet::~KTimetrackerApplet()
{
}

void KTimetrackerApplet::paintInterface(QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &rect)
{
  /* draw corners */
  QRect tmpRect( 0, 0, 20, 23 );
  m_theme->paint( p, tmpRect, "topleft" );

  tmpRect.setRect( m_size.width() - 21, 0, 20, 23 );
  m_theme->paint( p, tmpRect, "topright" );

  tmpRect.setRect( 0, m_size.height() - 24, 20, 23 );
  m_theme->paint( p, tmpRect, "bottomleft" );

  tmpRect.setRect( m_size.width() - 21, m_size.height() - 24, 20, 23 );
  m_theme->paint( p, tmpRect, "bottomright" );

  /* draw sides */
  tmpRect.setRect( 0, 22, 20, m_size.height() - 46 );
  m_theme->paint( p, tmpRect, "left" );

  tmpRect.setRect( m_size.width() - 21, 22, 20, m_size.height() - 46 );
  m_theme->paint( p, tmpRect, "right" );

  tmpRect.setRect( 19, 0, m_size.width () - 40, 23 );
  m_theme->paint( p, tmpRect, "top" );

  tmpRect.setRect( 19, m_size.height() - 24, m_size.width() - 40, 23 );
  m_theme->paint( p, tmpRect, "bottom" );

  /* draw center */
  tmpRect.setRect( 19, 22, m_size.width() - 40, m_size.height() - 46 );
  m_theme->paint( p, tmpRect, "center" );
}

#include "ktimetrackerapplet.moc"
