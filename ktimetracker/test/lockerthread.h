#include <QThread>

class QString;

/** 
 * Attempt to open and lock a calendar resource in a seperate thread.
 *
 * Result of attempt returned by gotlock().
 */
class LockerThread : public QThread
{
  public:
    LockerThread( const QString &filename );
    //void setIcsFile( const QString &filename );
    void run();
    bool gotlock() const { return m_gotlock; };

  private:
    QString m_icsfile;
    bool    m_gotlock;
};
