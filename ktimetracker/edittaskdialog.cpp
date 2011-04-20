#include "edittaskdialog.h"
#include "ui_edittaskdialog.h"
#include "historydialog.h"
#include <KMessageBox>
#include "ktimetrackerutility.h"
#include <QCheckBox>

QVector<QCheckBox*> desktopcheckboxes;

EditTaskDialog::EditTaskDialog(TaskView *parent, const QString &caption, DesktopList* desktopList)
  : QDialog( parent ),
    m_ui(new Ui::EditTaskDialog)
{
    setWindowTitle(caption);
    m_parent=parent;
    m_ui->setupUi(this);

    { // Set the desktop checkboxes
        QCheckBox* desktopcheckbox;
        desktopcheckboxes.clear();
        int lines=5;
        for (int i=0; i<desktopcount(); ++i)
        {
            desktopcheckbox = new QCheckBox(m_ui->autotrackinggroupbox);
            desktopcheckbox->setObjectName(QString::fromUtf8("desktop_").append(i));
            desktopcheckbox->setText(KWindowSystem::desktopName( i + 1 ));
            m_ui->gridLayout_2->addWidget(desktopcheckbox, i % lines, i / lines + 1);
            desktopcheckboxes.push_back(desktopcheckbox);
        }
        if ( desktopList && ( desktopList->size() > 0 ) )
        {
            DesktopList::iterator it = desktopList->begin();
            while ( it != desktopList->end() )
            {
                desktopcheckboxes[*it]->setChecked( true );
                ++it;
            }
            m_ui->autotrackinggroupbox->setChecked(true);
        }
        else
            for ( int i = 0; i < desktopcheckboxes.count(); i++ )
                desktopcheckboxes[i]->setEnabled(false);
    }
}

EditTaskDialog::~EditTaskDialog()
{
    delete m_ui;
}

void EditTaskDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
            m_ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

QString EditTaskDialog::taskName()
{
    return m_ui->tasknamelineedit->text();
}

QString EditTaskDialog::timeChange()
{
    return m_ui->letimechange->text();
}

void EditTaskDialog::setTask( const QString &name )
{
    m_ui->tasknamelineedit->setText( name );
}

void EditTaskDialog::status(DesktopList *desktopList) const
{
    if ( desktopList )
    {
        desktopList->clear();
        for ( int i = 0; i < desktopcheckboxes.count(); i++ )
        {
            if ( desktopcheckboxes[i]->isEnabled() && desktopcheckboxes[i]->isChecked() ) desktopList->append( i );
        }
    }
}

void EditTaskDialog::on_edittimespushbutton_clicked()
{
    historydialog* historydialog1=new historydialog(m_parent);
    lower();
    historydialog1->exec();
}

void EditTaskDialog::on_autotrackinggroupbox_clicked()
{
     for ( int i = 0; i < desktopcheckboxes.count(); i++ )
        desktopcheckboxes[i]->setEnabled(m_ui->autotrackinggroupbox->isChecked());
}
