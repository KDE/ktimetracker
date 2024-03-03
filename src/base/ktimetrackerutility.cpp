/*
    SPDX-FileCopyrightText: 2007 Thorsten Staerk <dev@staerk.de>
    SPDX-FileCopyrightText: 2019, 2021 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ktimetrackerutility.h"

#include <cmath>

#include <QLocale>

QString formatTime(double minutes, bool decimal)
{
    QString time;
    if (decimal) {
        time = QStringLiteral("%1").arg(minutes / 60.0, 0, 'f', 2);
        time.replace(QChar::fromLatin1('.'), QLocale().decimalPoint());
    } else {
        const auto absMinutes = static_cast<qlonglong>(std::round(std::fabs(minutes)));
        time = QStringLiteral("%1%2:%3")
                   .arg(minutes < 0 ? QString(QLocale().negativeSign()) : QStringLiteral())
                   .arg(absMinutes / 60)
                   .arg(absMinutes % 60, 2, 10, QLatin1Char('0'));
    }

    return time;
}

QString getCustomProperty(const KCalendarCore::Incidence::Ptr &incident, const QString &name)
{
    static const QByteArray eventAppName = QByteArray("ktimetracker");
    static const QByteArray eventAppNameOld = QByteArray("karm");
    const QByteArray nameArray = name.toLatin1();

    // If a KDE-karm-* exists and not KDE-ktimetracker-*, change this.
    if (incident->customProperty(eventAppName, nameArray).isNull()
        && !incident->customProperty(eventAppNameOld, nameArray).isNull()) {
        incident->setCustomProperty(eventAppName, nameArray, incident->customProperty(eventAppNameOld, nameArray));
    }

    return incident->customProperty(eventAppName, nameArray);
}
