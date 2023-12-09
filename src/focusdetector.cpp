/*
 * Copyright (C) 2007 by René Mérou <ochominutosdearco@gmail.com>
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

#include "focusdetector.h"

#include <KX11Extras>
#include <KWindowInfo>
#include <KWindowSystem>

#include "ktimetrackerutility.h"

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
