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
#include <QX11Info>

#include <KWindowSystem>
#include <KLocalizedString>
#include <KMessageBox>

IdleTimeDetector::IdleTimeDetector(int maxIdle)
{
    _maxIdle = maxIdle;

#if defined(HAVE_XSCREENSAVER)
    int event_base, error_base;
    _idleDetectionPossible = XScreenSaverQueryExtension(QX11Info::display(), &event_base, &error_base) != 0;

    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &IdleTimeDetector::check);
#else
    _idleDetectionPossible = false;
#endif // HAVE_XSCREENSAVER
}

bool IdleTimeDetector::isIdleDetectionPossible()
{
    return _idleDetectionPossible;
}

void IdleTimeDetector::check()
{
#if defined(HAVE_XSCREENSAVER)
    qCDebug(KTT_LOG) << "kompiled for libxss and x11, idledetectionpossible is " << _idleDetectionPossible;
    if (_idleDetectionPossible) {
        _mit_info = XScreenSaverAllocInfo();
        XScreenSaverQueryInfo(QX11Info::display(), QX11Info::appRootWindow(), _mit_info);
        idleminutes = (_mit_info->idle / 1000) / secsPerMinute;
        qCDebug(KTT_LOG) << "The desktop has been idle for " << idleminutes << " minutes.";
        qCDebug(KTT_LOG) << "The idle time in miliseconds is " << _mit_info->idle;
        if (idleminutes >= _maxIdle) {
            informOverrun();
        }
    }
#endif // HAVE_XSCREENSAVER
}

void IdleTimeDetector::setMaxIdle(int maxIdle)
{
    _maxIdle = maxIdle;
}

void IdleTimeDetector::revert()
{
    // revert and stop
    qCDebug(KTT_LOG) << "Entering function";
    QDateTime end = QDateTime::currentDateTime();
    const int diff = start.secsTo(end) / secsPerMinute;
    emit subtractTime(idleminutes + diff); // subtract the time that has been added on the display
    emit stopAllTimers(idlestart);
}

void IdleTimeDetector::informOverrun()
{
    if (!_overAllIdleDetect) {
        // In the preferences the user has indicated that he does not want idle detection.
        return;
    }

    _timer->stop();
    start = QDateTime::currentDateTime();
    idlestart = start.addSecs(-60 * _maxIdle);
    QString backThen = idlestart.time().toString();

    // Create dialog
    QString hintYes = i18n("Continue timing. Timing has started at %1", backThen);
    KGuiItem buttonYes(i18n("Continue timing."), QString(), hintYes, hintYes);

    QString hintNo = i18n("Stop timing and revert back to the time at %1.", backThen);
    KGuiItem buttonNo(i18n("Revert timing"), QString(), hintNo, hintNo);

    const auto result = KMessageBox::questionYesNo(
        nullptr,
        i18n("Desktop has been idle since %1. What do you want to do ?", backThen),
        QString(), buttonYes, buttonNo);
    switch (result) {
        case KMessageBox::Yes:
            _timer->start();
            break;
        default:
            qCWarning(KTT_LOG) << "unexpected button clicked" << result;
            Q_FALLTHROUGH();
        case KMessageBox::No:
            revert();
            break;
    }
}

void IdleTimeDetector::startIdleDetection()
{
#if defined(HAVE_XSCREENSAVER)
    if (!_timer->isActive()) {
        _timer->start(testInterval);
    }
#endif // HAVE_XSCREENSAVER
}

void IdleTimeDetector::stopIdleDetection()
{
#if defined(HAVE_XSCREENSAVER)
    if (_timer->isActive()) {
        _timer->stop();
    }
#endif // HAVE_XSCREENSAVER
}

void IdleTimeDetector::toggleOverAllIdleDetection(bool on)
{
    _overAllIdleDetect = on;
}
