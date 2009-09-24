#ifndef EDITTASKDIALOG_H
#define EDITTASKDIALOG_H

#include <QtGui/QDialog>
#include "desktoplist.h"
#include "taskview.h"

namespace Ui
{
    class EditTaskDialog;
}

class EditTaskDialog : public QDialog
{
    Q_OBJECT
public:
    EditTaskDialog( TaskView *parent, const QString &caption, DesktopList* desktopList = 0 );
    ~EditTaskDialog();
    QString taskName();
    void setTask( const QString &name, long time, long sessionTime );
    void status( DesktopList *desktopList) const;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::EditTaskDialog *m_ui;
    TaskView *m_parent;

private slots:
    void on_edittimespushbutton_clicked();
};

#endif // EDITTASKDIALOG_H
