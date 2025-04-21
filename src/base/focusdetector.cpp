/*
    SPDX-FileCopyrightText: 2007 René Mérou <ochominutosdearco@gmail.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "focusdetector.h"

#ifdef Q_OS_LINUX
#include <KWindowInfo>
#include <KWindowSystem>
#include <KX11Extras>
#endif

FocusDetector::FocusDetector()
{
#ifdef Q_OS_LINUX
    if (KWindowSystem::isPlatformX11()) {
        connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &FocusDetector::onFocusChanged);
    }
#endif
}

void FocusDetector::onFocusChanged(WId /*unused*/)
{
#ifdef Q_OS_LINUX
    if (KWindowSystem::isPlatformX11()) {
        Q_EMIT newFocus(KWindowInfo(KX11Extras::activeWindow(), NET::WMName).name());
    }
#endif
}

#include "moc_focusdetector.cpp"
