/*
 * Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
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

#include <KIdleTime>
#include <KLocalizedString>
#include <KMessageBox>

#include "ktimetrackerutility.h" // SecsPerMinute
#include "ktt_debug.h"

IdleTimeDetector::IdleTimeDetector(int maxIdle)
    : m_overAllIdleDetect(false)
    , m_maxIdle(maxIdle)
    , m_timeoutId(0)
{
    connect(
        KIdleTime::instance(), QOverload<int, int>::of(&KIdleTime::timeoutReached),
        this, &IdleTimeDetector::timeoutReached);
}

// static
bool IdleTimeDetector::isIdleDetectionPossible()
{
    int id = KIdleTime::instance()->addIdleTimeout(5000);
    if (id) {
        KIdleTime::instance()->removeIdleTimeout(id);
        return true;
    } else {
        return false;
    }
}

void IdleTimeDetector::timeoutReached(int /*unused*/, int timeout)
{
    qCDebug(KTT_LOG) << "The desktop has been idle for " << timeout << " msec.";

    if (!m_overAllIdleDetect) {
        // In the preferences the user has indicated that he does not want idle detection.
        return;
    }

    stopIdleDetection();

    // when the idletimedetectordialog started
    QDateTime dialogStart = QDateTime::currentDateTime();

    // when the idleness started
    QDateTime idleStart = dialogStart.addSecs(-secsPerMinute * m_maxIdle);
    QString backThen = idleStart.time().toString();

    // Create dialog
    QString hintYes = i18nc(
        "@info:tooltip", "Apply the idle time since %1 to all active\ntimers and keep them running.", backThen);
    KGuiItem buttonYes(i18nc("@action:button", "Continue Timing"), QString(), hintYes, hintYes);

    QString hintNo = i18nc("@info:tooltip", "Stop timing and revert back to the time at %1", backThen);
    KGuiItem buttonNo(i18nc("@action:button", "Revert Timing"), QString(), hintNo, hintNo);

    const auto result = KMessageBox::questionYesNo(
        nullptr,
        i18n("Desktop has been idle since %1. What do you want to do?", backThen),
        QString(), buttonYes, buttonNo);
    switch (result) {
        case KMessageBox::Yes:
            startIdleDetection();
            break;
        default:
            qCWarning(KTT_LOG) << "unexpected button clicked" << result;
            Q_FALLTHROUGH();
        case KMessageBox::No:
            revert(dialogStart, idleStart, timeout / 1000 / secsPerMinute);
            break;
    }
}

void IdleTimeDetector::setMaxIdle(int maxIdle)
{
    m_maxIdle = maxIdle;
}

void IdleTimeDetector::revert(const QDateTime &dialogStart, const QDateTime &idleStart, int idleMinutes)
{
    // revert and stop
    QDateTime end = QDateTime::currentDateTime();
    const int64_t diff = dialogStart.secsTo(end) / secsPerMinute;
    emit subtractTime(idleMinutes + diff); // subtract the time that has been added on the display
    emit stopAllTimers(idleStart);
}

void IdleTimeDetector::startIdleDetection()
{
    if (!m_timeoutId) {
        m_timeoutId = KIdleTime::instance()->addIdleTimeout(m_maxIdle * secsPerMinute * 1000);
    }
}

void IdleTimeDetector::stopIdleDetection()
{
    if (m_timeoutId) {
        KIdleTime::instance()->removeIdleTimeout(m_timeoutId);
        m_timeoutId = 0;
    }
}

void IdleTimeDetector::toggleOverAllIdleDetection(bool on)
{
    m_overAllIdleDetect = on;
}
