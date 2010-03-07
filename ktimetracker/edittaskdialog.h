#ifndef EDITTASKDIALOG_H
#define EDITTASKDIALOG_H

#include <QtGui/QDialog>
#include "desktoplist.h"
#include "taskview.h"

namespace Ui
{
    class EditTaskDialog;
}

/**
 * Class to show a dialog in ktimetracker to enter data about a task.
 *
 * Holds task name and auto-tracking virtual desktops.
 *
 * Typically this class will be called by a taskview object to enter a new task
 * or to edit an existing task. After quitting, you can read out task name and all
 * properties till you delete the object.
 *
 * @short Class to show a dialog to enter data about a task
 * @author Thorsten Staerk
 */

class EditTaskDialog : public QDialog
{
    Q_OBJECT
public:
    EditTaskDialog( TaskView *parent, const QString &caption, DesktopList* desktopList = 0 );
    ~EditTaskDialog();
    QString taskName();
    QString timeChange();
    void setTask( const QString &name );
    void status( DesktopList *desktopList) const;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::EditTaskDialog *m_ui;
    TaskView *m_parent;

private slots:
    void on_autotrackinggroupbox_clicked();
    void on_edittimespushbutton_clicked();
};

#endif // EDITTASKDIALOG_H
