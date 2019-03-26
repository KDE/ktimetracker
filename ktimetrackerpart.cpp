/*
 *     Copyright (C) 2005 by Thorsten Staerk <kde@staerk.de>
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

// TODO: what does totalTimesChanged()?

#include "ktimetrackerpart.h"

#include <QMenu>

#include <KLocalizedString>
#include <KActionCollection>
#include <KShortcutsDialog>
#include <KPluginFactory>

#include "ktimetrackerutility.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "ktimetracker-version.h"
#include "ktimetracker.h"
#include "timetrackerwidget.h"

K_PLUGIN_FACTORY(ktimetrackerPartFactory, registerPlugin<KTimeTrackerPart>();)
K_EXPORT_PLUGIN( ktimetrackerPartFactory("ktimetracker","ktimetracker") )

KTimeTrackerPart::KTimeTrackerPart(QWidget *parentWidget, QObject *parent, const QVariantList&)
    : KParts::ReadWritePart(parent)
{
    qCDebug(KTT_LOG) << "Entering function";
    // we need an instance
    m_mainWidget = new TimeTrackerWidget(parentWidget);
    setWidget(m_mainWidget);
    setXMLFile("ktimetrackerui.rc");
    makeMenus();
}

void KTimeTrackerPart::makeMenus()
{
    m_mainWidget->setupActions(actionCollection());
    QAction *actionKeyBindings;
    actionKeyBindings = KStandardAction::keyBindings(this, &KTimeTrackerPart::keyBindings, actionCollection());
    // Tool tips must be set after the createGUI.
    actionKeyBindings->setToolTip( i18n("Configure key bindings") );
    actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which are specific to ktimetracker") );
}

void KTimeTrackerPart::keyBindings()
{
  KShortcutsDialog::configure( actionCollection(),
                               KShortcutsEditor::LetterShortcutsAllowed );
}

void KTimeTrackerPart::setStatusBar(const QString& qs)
{
    qCDebug(KTT_LOG) << "Entering function";
    emit setStatusBarText(qs);
}

bool KTimeTrackerPart::openFile(const QString& path)
{
    m_mainWidget->openFile(path);
    emit setWindowCaption(path);

    // connections
    connect(m_mainWidget, &TimeTrackerWidget::statusBarTextChangeRequested, this, &KTimeTrackerPart::setStatusBar);
    connect(m_mainWidget, &TimeTrackerWidget::setCaption, this, &KTimeTrackerPart::setWindowCaption);
    return true;
}

bool KTimeTrackerPart::openFile()
{
    return openFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + QStringLiteral("ktimetracker/ktimetracker.ics"));
}

bool KTimeTrackerPart::saveFile()
{
    m_mainWidget->saveFile();
    return true;
}

#include "ktimetrackerpart.moc"
