#include "ktimetrackerengine.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTimer>

KTimetrackerEngine::KTimetrackerEngine( QObject* parent, const QStringList& args )
{
  Q_UNUSED( args );
  m_timer = new QTimer( this );
  m_timer->setSingleShot( false );
  connect( m_timer, SIGNAL( timeout() ),
           this, SLOT( updateData() ) );
}

bool KTimetrackerEngine::sourceRequested( const QString &name )
{
  if ( name != "timetracker" ) {
    return false;
  } else {
    updateData();

    if ( !m_timer->isActive() ) {
      m_timer->start();
    }

    return true;
  }
}

void KTimetrackerEngine::updateData()
{
  QDBusInterface iface( "org.kde.karm", "/Karm", "org.kde.karm.Karm", QDBusConnection::sessionBus() );
  QDBusReply<QStringList> reply;
  bool valid = iface.isValid();
  if ( valid ) {
    if ( ( reply = iface.call("getActiveTasks") ).isValid() ) {
      setData( "timetracker", "activetasks", reply.value() );
    }
  }
  setData( "timetracker", "ktimetracker-running", valid );

  checkForUpdates();
}

#include "ktimetrackerengine.moc"
