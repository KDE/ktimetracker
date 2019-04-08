#ifndef KTIMETRACKER_ICALFORMATKIO_H
#define KTIMETRACKER_ICALFORMATKIO_H

#include <KCalCore/ICalFormat>

class ICalFormatKIO : public KCalCore::ICalFormat
{
public:
    ICalFormatKIO();
    ~ICalFormatKIO() override = default;

    bool load(const KCalCore::Calendar::Ptr &calendar, const QString &fileName) override;
    bool save(const KCalCore::Calendar::Ptr &calendar, const QString &fileName) override;
};

#endif // KTIMETRACKER_ICALFORMATKIO_H
