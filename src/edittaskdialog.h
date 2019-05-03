#ifndef EDITTASKDIALOG_H
#define EDITTASKDIALOG_H

#include <QDialog>

#include "desktoplist.h"
#include "ui_edittaskdialog.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

class TaskView;

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
    EditTaskDialog(TaskView *parent, const QString &caption, DesktopList *desktopList = nullptr);
    ~EditTaskDialog() override = default;

    QString taskName();
    QString taskDescription();
    QString timeChange();
    void setTask(const QString &name);
    void setDescription(const QString &description);
    void status(DesktopList *desktopList) const;

protected:
    void changeEvent(QEvent *e) override;

private:
    TaskView *m_parent;
    Ui::EditTaskDialog m_ui;
    QVector<QCheckBox*> m_desktopCheckboxes;

private Q_SLOTS:
    void on_autotrackinggroupbox_clicked();
    void on_edittimespushbutton_clicked();
};

#endif // EDITTASKDIALOG_H
