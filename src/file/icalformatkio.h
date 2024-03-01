/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_ICALFORMATKIO_H
#define KTIMETRACKER_ICALFORMATKIO_H

#include <KCalendarCore/ICalFormat>

class ICalFormatKIO : public KCalendarCore::ICalFormat
{
public:
    ICalFormatKIO() = default;
    ~ICalFormatKIO() override = default;

    /**
     * Read calendar from local or remote file.
     *
     * @param calendar
     * @param urlString Must start with a schema, for example "file:///" or "https://"
     * @return
     */
    bool load(const KCalendarCore::Calendar::Ptr &calendar, const QString &urlString) override;

    /**
     * Write calendar to local or remote file.
     *
     * @param calendar
     * @param urlString Must start with a schema, for example "file:///" or "https://"
     * @return
     */
    bool save(const KCalendarCore::Calendar::Ptr &calendar, const QString &urlString) override;
};

#endif // KTIMETRACKER_ICALFORMATKIO_H
