/*
    This file is part of ktimetracker.
    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Thorsten Staerk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <QVBoxLayout>

#include <kaboutdata.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include "ktimetrackerconfigwidget.h"
#include "kcmktimetrackerconfig.h"

#include <kdemacros.h>
#include <kgenericfactory.h>

K_PLUGIN_FACTORY(KCMKTimeTrackerConfigFactory, registerPlugin<KCMKTimeTrackerConfig>();)
K_EXPORT_PLUGIN(KCMKTimeTrackerConfigFactory( "kcmktimetrackerconfig" ))


KCMKTimeTrackerConfig::KCMKTimeTrackerConfig( QWidget *parent, const QVariantList & )
  : KCModule( KCMKTimeTrackerConfigFactory::componentData(), parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  mConfigWidget = new KTimeTrackerConfigWidget( this, "mConfigWidget" );
  layout->addWidget( mConfigWidget );

  connect( mConfigWidget, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );

  load();

  KAboutData *about = new KAboutData( I18N_NOOP( "kcmktimetrackerconfig" ), 0,
                                      ki18n( "ktimetracker Configure Dialog" ),
                                      0, KLocalizedString(), KAboutData::License_GPL,
                                      ki18n( "(c), 2003 - 2008 Tobias Koenig" ) );

  about->addAuthor( ki18n("Tobias Koenig"), KLocalizedString(), "tokoe@kde.org" );
  about->addAuthor( ki18n("Thorsten Staerk"), KLocalizedString(), "kde@staerk.de" );
  setAboutData( about );
}

void KCMKTimeTrackerConfig::load()
{
  mConfigWidget->restoreSettings();
}

void KCMKTimeTrackerConfig::save()
{
  mConfigWidget->saveSettings();
}

void KCMKTimeTrackerConfig::defaults()
{
  mConfigWidget->defaults();
}

#include "kcmktimetrackerconfig.moc"
