/*
 *     Copyright (C) 2003 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#ifndef KARM_PRINT_DIALOG_H
#define KARM_PRINT_DIALOG_H

#include <KDialog>

class QCheckBox;

class KComboBox;
namespace KPIM {
  class KDateEdit;
};

class PrintDialog : public KDialog
{
  Q_OBJECT

  public:
    PrintDialog();

    /* Return the from date entered.  */
    QDate from() const;

    /* Return the to date entered.  */
    QDate to() const;

    /* Whether to summarize per week */
    bool perWeek() const;

    /* Whether to print all tasks */
    bool allTasks() const;

    /* Whether to print totals only, instead of per-day columns */
    bool totalsOnly() const;

private:
    KPIM::KDateEdit *_from, *_to;
    QCheckBox *_perWeek;
    KComboBox *_allTasks;
    QCheckBox *_totalsOnly;
};

#endif // KARM_PRINT_DIALOG_H

