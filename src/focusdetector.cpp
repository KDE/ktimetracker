/*
    SPDX-FileCopyrightText: 2007 René Mérou <ochominutosdearco@gmail.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "focusdetector.h"

#include <KX11Extras>
#include <KWindowInfo>
#include <KWindowSystem>


FocusDetector::FocusDetector()
{
    if(KWindowSystem::isPlatformX11())
    {
        connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &FocusDetector::onFocusChanged);
    }
}

void FocusDetector::onFocusChanged(WId /*unused*/)
{
    if(KWindowSystem::isPlatformX11())
    {
        Q_EMIT newFocus(KWindowInfo(KX11Extras::activeWindow(), NET::WMName).name());
    }
}

#include "moc_focusdetector.cpp"
