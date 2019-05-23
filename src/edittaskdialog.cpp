#include "edittaskdialog.h"

#include <QCheckBox>

#include <KMessageBox>
#include <KWindowSystem>

#include "historydialog.h"
#include "ktimetrackerutility.h"
#include "taskview.h"
#include "widgets/taskswidget.h"

EditTaskDialog::EditTaskDialog(TaskView *parent, const QString &caption, DesktopList *desktopList)
    : QDialog(parent->tasksWidget())
    , m_ui()
    , m_desktopCheckboxes()
{
    setWindowTitle(caption);
    m_parent = parent;
    m_ui.setupUi(this);

    // Set the desktop checkboxes
    const int lines = 5;
    for (int i = 0; i < KWindowSystem::numberOfDesktops(); ++i) {
        auto *checkbox = new QCheckBox(m_ui.autotrackinggroupbox);
        checkbox->setObjectName(QString::fromUtf8("desktop_").append(i));
        checkbox->setText(KWindowSystem::desktopName(i + 1));
        m_ui.gridLayout_2->addWidget(checkbox, i % lines, i / lines + 1);
        m_desktopCheckboxes.push_back(checkbox);
    }

    if (desktopList && !desktopList->empty()) {
        for (int desktop : *desktopList) {
            m_desktopCheckboxes[desktop]->setChecked(true);
        }

        m_ui.autotrackinggroupbox->setChecked(true);
    } else {
        for (QCheckBox *checkbox : m_desktopCheckboxes) {
            checkbox->setEnabled(false);
        }
    }
}

void EditTaskDialog::changeEvent(QEvent *e)
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

void EditTaskDialog::setTask(const QString &name)
{
    m_ui.tasknamelineedit->setText(name);
}

void EditTaskDialog::setDescription(const QString &description)
{
    m_ui.tasknametextedit->setText(description);
}

void EditTaskDialog::status(DesktopList *desktopList) const
{
    if (desktopList) {
        desktopList->clear();
        for (int i = 0; i < m_desktopCheckboxes.count(); ++i) {
            if (m_desktopCheckboxes[i]->isEnabled() && m_desktopCheckboxes[i]->isChecked()) {
                desktopList->append(i);
            }
        }
    }
}

void EditTaskDialog::on_edittimespushbutton_clicked()
{
    auto* dialog = new HistoryDialog(m_parent->tasksWidget(), m_parent->storage()->projectModel());
    connect(dialog, &HistoryDialog::timesChanged, m_parent, &TaskView::reFreshTimes);
    lower();
    dialog->exec();
}

void EditTaskDialog::on_autotrackinggroupbox_clicked()
{
    for (QCheckBox *checkbox : m_desktopCheckboxes) {
        checkbox->setEnabled(m_ui.autotrackinggroupbox->isChecked());
     }
}
