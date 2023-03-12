/*
 * Copyright (C) 2007 by Thorsten Staerk <dev@staerk.de>
 * Copyright (C) 2019, 2021  Alexander Potashev <aspotashev@gmail.com>
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

QString getCustomProperty(const KCalCore::Incidence::Ptr &incident, const QString &name)
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
