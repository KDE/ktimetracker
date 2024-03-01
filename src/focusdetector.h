/*
    SPDX-FileCopyrightText: 2007 René Mérou <ochominutosdearco@gmail.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_FOCUS_DETECTOR_H
#define KTIMETRACKER_FOCUS_DETECTOR_H

#include <QObject>
#include <QWidgetSet>

/**
  Keep track of what window has the focus.
 */
class FocusDetector : public QObject
{
    Q_OBJECT

public:
    /**
      Initializes the time period
     */
    FocusDetector();

public Q_SLOTS:
    void onFocusChanged(WId id);

Q_SIGNALS:
    void newFocus(const QString &windowName);
};

#endif // KTIMETRACKER_FOCUS_DETECTOR_H
