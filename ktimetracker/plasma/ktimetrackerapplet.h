/*
 *   Copyright (C) 2007 Mathias Soeken <msoeken@tzi.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KTIMETRACKERAPPLET_H
#define KTIMETRACKERAPPLET_H

#include <QImage>
#include <QPaintDevice>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QPaintEvent>
#include <QPainter>
#include <QTime>
#include <QX11Info>
#include <QGraphicsItem>
#include <QColor>

#include <plasma/applet.h>
#include <plasma/dataengine.h>
#include "ui_clockConfig.h"

class QTimer;

class KDialog;

namespace Plasma
{
  class Svg;
  class LineEdit;
}

class KTimetrackerApplet : public Plasma::Applet
{
Q_OBJECT
public:
  KTimetrackerApplet(QObject *parent, const QStringList &args);
  ~KTimetrackerApplet();

  void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
  QSizeF contentSize() const;
  void constraintsUpdated();

public Q_SLOTS:
  void updated(const QString &name, const Plasma::DataEngine::Data &data);

private:
  QSizeF m_size;
  int m_pixelSize;
  QString m_timezone;
  Plasma::Svg* m_theme;
  Plasma::LineEdit* m_label;
};

K_EXPORT_PLASMA_APPLET(timetrackerapplet, KTimetrackerApplet)

#endif
