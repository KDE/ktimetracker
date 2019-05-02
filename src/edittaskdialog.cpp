#include "edittaskdialog.h"

#include <QCheckBox>

#include <KMessageBox>
#include <KWindowSystem>

#include "historydialog.h"
#include "ktimetrackerutility.h"
#include "taskview.h"

QVector<QCheckBox*> desktopcheckboxes;

EditTaskDialog::EditTaskDialog(TaskView* parent, const QString& caption, DesktopList* desktopList)
    : QDialog(parent)
    , m_ui()
{
    setWindowTitle(caption);
    m_parent = parent;
    m_ui.setupUi(this);

    { // Set the desktop checkboxes
        QCheckBox* desktopcheckbox;
        desktopcheckboxes.clear();
        int lines = 5;
        for (int i = 0; i < KWindowSystem::numberOfDesktops(); ++i) {
            desktopcheckbox = new QCheckBox(m_ui.autotrackinggroupbox);
            desktopcheckbox->setObjectName(QString::fromUtf8("desktop_").append(i));
            desktopcheckbox->setText(KWindowSystem::desktopName( i + 1 ));
            m_ui.gridLayout_2->addWidget(desktopcheckbox, i % lines, i / lines + 1);
            desktopcheckboxes.push_back(desktopcheckbox);
        }

        if (desktopList && desktopList->size() > 0) {
            DesktopList::iterator it = desktopList->begin();
            while (it != desktopList->end()) {
                desktopcheckboxes[*it]->setChecked(true);
                ++it;
            }

            m_ui.autotrackinggroupbox->setChecked(true);
        } else {
            for (int i = 0; i < desktopcheckboxes.count(); ++i) {
                desktopcheckboxes[i]->setEnabled(false);
            }
        }
    }
}

void EditTaskDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            m_ui.retranslateUi(this);
            break;
        default:
            break;
    }
}

QString EditTaskDialog::taskName()
{
    return m_ui.tasknamelineedit->text();
}

QString EditTaskDialog::taskDescription()
{
    return m_ui.tasknametextedit->toPlainText();
}

QString EditTaskDialog::timeChange()
{
    return m_ui.letimechange->text();
}

void EditTaskDialog::setTask(const QString& name)
{
    m_ui.tasknamelineedit->setText(name);
}

void EditTaskDialog::setDescription(const QString& description)
{
    m_ui.tasknametextedit->setText(description);
}

void EditTaskDialog::status(DesktopList *desktopList) const
{
    if (desktopList) {
        desktopList->clear();
        for (int i = 0; i < desktopcheckboxes.count(); ++i) {
            if (desktopcheckboxes[i]->isEnabled() && desktopcheckboxes[i]->isChecked()) {
                desktopList->append(i);
            }
        }
    }
}

void EditTaskDialog::on_edittimespushbutton_clicked()
{
    auto* dialog = new HistoryDialog(m_parent, m_parent->storage());
    connect(dialog, &HistoryDialog::timesChanged, m_parent, &TaskView::reFreshTimes);
    lower();
    dialog->exec();
}

void EditTaskDialog::on_autotrackinggroupbox_clicked()
{
     for (int i = 0; i < desktopcheckboxes.count(); ++i) {
         desktopcheckboxes[i]->setEnabled(m_ui.autotrackinggroupbox->isChecked());
     }
}
