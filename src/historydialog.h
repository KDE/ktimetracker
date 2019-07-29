/*
 * Copyright (C) 2009 by Thorsten Staerk and Mathias Soeken <msoeken@tzi.de>
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

#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include <QDialog>

#include "ui_historydialog.h"

class ProjectModel;

class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(QWidget *parent, ProjectModel *projectModel);
    ~HistoryDialog() override = default;

protected:
    void changeEvent(QEvent *e) override;

private Q_SLOTS:
    /**
     * The historywidget contains all events and can be shown with File | Edit History.
     * The user can change dates and comments in there.
     * A change triggers this procedure, it shall store the new values in the calendar.
     */
    void on_deletepushbutton_clicked();
    void onCellChanged(int row, int col);
    void on_buttonbox_rejected();

private:
    QString listAllEvents();
    QString refresh();

    ProjectModel *m_projectModel;
    Ui::HistoryDialog m_ui;
};

#endif // HISTORYDIALOG_H
