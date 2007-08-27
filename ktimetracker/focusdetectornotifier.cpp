/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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
#include "focusdetectornotifier.h"

#include "focusdetector.h"
#include "taskview.h"

//@cond PRIVATE
class FocusDetectorNotifier::Private {
  public:
    Private() {
      mDetector = new FocusDetector( 1 );
    }

    FocusDetector *mDetector;
    QList< TaskView * > mViews;
};
//@endcond

FocusDetectorNotifier* FocusDetectorNotifier::mInstance = 0;

FocusDetectorNotifier::FocusDetectorNotifier( QObject *parent ) 
  : QObject( parent ), d( new Private() )
{
}

FocusDetectorNotifier* FocusDetectorNotifier::instance()
{
  if ( !mInstance ) {
    mInstance = new FocusDetectorNotifier;
  }

  return mInstance;
}

FocusDetectorNotifier::~FocusDetectorNotifier()
{
  delete d;
}

void FocusDetectorNotifier::attach( TaskView *view )
{
  d->mViews.append( view );
  if ( d->mViews.count() == 1 ) {
    d->mDetector->startFocusDetection();
  }
}

void FocusDetectorNotifier::detach( TaskView *view )
{
  d->mViews.removeAll( view );
  if ( d->mViews.count() == 0) {
    d->mDetector->stopFocusDetection();
  }
}

FocusDetector *FocusDetectorNotifier::focusDetector() const
{
  return d->mDetector;
}
