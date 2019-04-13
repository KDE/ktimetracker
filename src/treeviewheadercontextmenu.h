/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

#ifndef TREEVIEWHEADERCONTEXTMENU_H
#define TREEVIEWHEADERCONTEXTMENU_H

#include <QObject>
#include <QPoint>
#include <QHash>
#include <QVector>
#include <QMenu>

class QAction;
class QTreeView;

/**
 * ContextMenu for QTreeView::header() to toggle the
 * visible state of the columns.
 *
 * It is possible to exclude columns from inserting in the
 * menu either by the @p excludedColumns parameter in the constructor.
 *
 * You can also change the display style of the items in the menu.
 *
 * @author Mathias Soeken <msoeken@tzi.de>
 */
class TreeViewHeaderContextMenu : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int style READ style)
    Q_PROPERTY(QMenu* menu READ menu)

public:
    enum { AlwaysCheckBox, CheckBoxOnChecked, ShowHideText };

public:
    TreeViewHeaderContextMenu(QObject* parent, QTreeView* widget, int style = AlwaysCheckBox, QVector<int> &&excludedColumns = QVector<int>());
    ~TreeViewHeaderContextMenu() override;

    int style() const { return mStyle; }
    QMenu *menu() const { return mContextMenu; }

private:
    void updateAction(QAction* action, int column);

private Q_SLOTS:
    void slotCustomContextMenuRequested(const QPoint&);

protected Q_SLOTS:
    void updateActions();
    void slotTriggered(QAction*);
    void slotAboutToShow();

protected:
    QTreeView *mWidget;
    QVector<QAction*> mActions;
    QMenu *mContextMenu;
    int mStyle;
    QHash<QAction*, int> mActionColumnMapping;
    QVector<int> mExcludedColumns;

Q_SIGNALS:
    void columnToggled(int);
};

#endif
