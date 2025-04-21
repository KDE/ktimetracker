/*
    SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "treeviewheadercontextmenu.h"

#include <QAction>
#include <QHeaderView>
#include <QTreeView>

#include <KLocalizedString>

#include "ktt_debug.h"

TreeViewHeaderContextMenu::TreeViewHeaderContextMenu(QObject *parent, QTreeView *widget, QVector<int> &&excludedColumns)
    : QObject(parent)
    , m_widget(widget)
    , m_contextMenu(nullptr)
    , m_excludedColumns(excludedColumns)
{
    m_widget->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_widget->header(), &QHeaderView::customContextMenuRequested, this, &TreeViewHeaderContextMenu::slotCustomContextMenuRequested);

    m_contextMenu = new QMenu(m_widget);
    m_contextMenu->addSection(i18nc("@title:menu", "Columns"));
    connect(m_contextMenu, &QMenu::triggered, this, &TreeViewHeaderContextMenu::slotTriggered);
    connect(m_contextMenu, &QMenu::aboutToShow, this, &TreeViewHeaderContextMenu::slotAboutToShow);
    updateActions();
}

TreeViewHeaderContextMenu::~TreeViewHeaderContextMenu()
{
    qDeleteAll(m_actions);
}

void TreeViewHeaderContextMenu::slotCustomContextMenuRequested(const QPoint &pos)
{
    qCDebug(KTT_LOG) << "Entering function";
    if (m_widget && m_contextMenu) {
        m_contextMenu->exec(m_widget->mapToGlobal(pos));
    }
}

void TreeViewHeaderContextMenu::updateActions()
{
    qCDebug(KTT_LOG) << "Entering function";
    if (m_widget) {
        for (QAction *action : m_actions) {
            m_contextMenu->removeAction(action);
        }

        m_actionColumnMapping.clear();
        qDeleteAll(m_actions);
        m_actions.clear();

        for (int c = 0; c < m_widget->model()->columnCount(); ++c) {
            if (m_excludedColumns.contains(c)) {
                continue;
            }

            auto *action = new QAction(this);
            updateAction(action, c);
            m_actions.append(action);

            m_contextMenu->addAction(action);
            m_actionColumnMapping[action] = c;
        }
    }
}

void TreeViewHeaderContextMenu::slotTriggered(QAction *action)
{
    qCDebug(KTT_LOG) << "Entering function";
    if (m_widget && action) {
        int column = m_actionColumnMapping[action];
        bool hidden = m_widget->isColumnHidden(column);
        m_widget->setColumnHidden(column, !hidden);
        updateAction(action, column);
        Q_EMIT columnToggled(column);
    }
}

void TreeViewHeaderContextMenu::slotAboutToShow()
{
    qCDebug(KTT_LOG) << "Entering function";
    for (QAction *action : m_actions) {
        updateAction(action, m_actionColumnMapping[action]);
    }
}

void TreeViewHeaderContextMenu::updateAction(QAction *action, int column)
{
    action->setCheckable(true);
    action->setChecked(!m_widget->isColumnHidden(column));
    action->setText(m_widget->model()->headerData(column, Qt::Horizontal).toString());
}

#include "moc_treeviewheadercontextmenu.cpp"
