/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
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

#include "idletimedetector.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include <KDialog>
#include <KGlobal>
#include <KLocale>    // i18n

#include <kdebug.h>
#include <KWindowSystem>

#ifdef Q_WS_X11
#include <QX11Info>
#endif

IdleTimeDetector::IdleTimeDetector(int maxIdle)
{
    _maxIdle = maxIdle;

#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
    int event_base, error_base;
    if(XScreenSaverQueryExtension(QX11Info::display(), &event_base, &error_base)) _idleDetectionPossible = true;
    else _idleDetectionPossible = false;
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(check()));
#else
    _idleDetectionPossible = false;
#endif // HAVE_LIBXSS
}

bool IdleTimeDetector::isIdleDetectionPossible()
{
    return _idleDetectionPossible;
}

void IdleTimeDetector::check()
{
    kDebug(5970) << "Entering function";
#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
    kDebug(5970) << "kompiled for libxss and x11, idledetectionpossible is " << _idleDetectionPossible;
    if (_idleDetectionPossible)
    {
        _mit_info = XScreenSaverAllocInfo();
        XScreenSaverQueryInfo(QX11Info::display(), QX11Info::appRootWindow(), _mit_info);
        idleminutes = (_mit_info->idle/1000)/secsPerMinute;
        kDebug(5970) << "The desktop has been idle for " << idleminutes << " minutes.";
        kDebug(5970) << "The idle time in miliseconds is " << _mit_info->idle;
        if (idleminutes >= _maxIdle)
        informOverrun();
    }
#endif // HAVE_LIBXSS
}

void IdleTimeDetector::setMaxIdle(int maxIdle)
{
    _maxIdle = maxIdle;
}

void IdleTimeDetector::revert()
{
    // revert and stop
    kDebug(5970) << "Entering function";
    QDateTime end = QDateTime::currentDateTime();
    int diff = start.secsTo(end)/secsPerMinute;
    emit(extractTime(idleminutes+diff)); // subtract the time that has been added on the display
    emit(stopAllTimers(idlestart));
}

#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
void IdleTimeDetector::informOverrun()
{
    if (!_overAllIdleDetect)
        return; // In the preferences the user has indicated that he do not
            // want idle detection.

    _timer->stop();
    start = QDateTime::currentDateTime();
    idlestart = start.addSecs(-60 * _maxIdle);
    QString backThen = KGlobal::locale()->formatTime(idlestart.time());
    // Create dialog
        KDialog *dialog=new KDialog( 0 );
        QWidget* wid=new QWidget(dialog);
        dialog->setMainWidget( wid );
        QVBoxLayout *lay1 = new QVBoxLayout(wid);
        QHBoxLayout *lay2 = new QHBoxLayout();
        lay1->addLayout(lay2);
        QString idlemsg=QString( "Desktop has been idle since %1. What do you want to do ?" ).arg(backThen);
        QLabel *label = new QLabel( idlemsg, wid );
        lay2->addWidget( label );
        connect( dialog , SIGNAL(cancelClicked()) , this , SLOT(revert()) );
        connect( wid , SIGNAL(changed(bool)) , wid , SLOT(enabledButtonApply(bool)) );
        QString explanation=i18n("Continue timing. Timing has started at %1", backThen);
        QString explanationrevert=i18n("Stop timing and revert back to the time at %1.", backThen);
        dialog->setButtonText(KDialog::Ok, i18n("Continue timing."));
        dialog->setButtonText(KDialog::Cancel, i18n("Revert timing"));
        dialog->setButtonWhatsThis(KDialog::Ok, explanation);
        dialog->setButtonWhatsThis(KDialog::Cancel, explanationrevert);
        // The user might be looking at another virtual desktop as where ktimetracker is running
        KWindowSystem::self()->setOnDesktop( dialog->winId(), KWindowSystem::self()->currentDesktop() );
        KWindowSystem::self()->demandAttention( dialog->winId() );
        kDebug(5970) << "Setting WinId " << dialog->winId() << " to deskTop " << KWindowSystem::self()->currentDesktop();
        dialog->show();
}
#endif // HAVE_LIBXSS

void IdleTimeDetector::startIdleDetection()
{
#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
    if (!_timer->isActive())
        _timer->start(testInterval);
#endif //HAVE_LIBXSS
}

void IdleTimeDetector::stopIdleDetection()
{
#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
    if (_timer->isActive())
        _timer->stop();
#endif // HAVE_LIBXSS
}

void IdleTimeDetector::toggleOverAllIdleDetection(bool on)
{
    _overAllIdleDetect = on;
}

#include "idletimedetector.moc"
