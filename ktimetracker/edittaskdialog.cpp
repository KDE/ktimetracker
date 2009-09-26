#include "edittaskdialog.h"
#include "ui_edittaskdialog.h"
#include "historydialog.h"
#include <KMessageBox>
#include "ktimetrackerutility.h"

EditTaskDialog::EditTaskDialog( TaskView *parent, const QString &caption, DesktopList* desktopList)
  : QDialog( parent ),
    m_ui(new Ui::EditTaskDialog)
{
    setWindowTitle(caption);
    m_parent=parent;
    m_ui->setupUi(this);

    // Set the desktop checkboxes
        QVector<QCheckBox*> desktopcheckboxes;
        QCheckBox* desktopcheckbox;
        int lines=5;
        for (int i=0; i<desktopcount(); ++i)
        {
            desktopcheckbox = new QCheckBox(m_ui->autotrackinggroupbox);
            desktopcheckbox->setObjectName(QString::fromUtf8("desktop_").append(i));
            desktopcheckbox->setText(KWindowSystem::desktopName( i + 1 ));
            m_ui->gridLayout_2->addWidget(desktopcheckbox, i / lines + 1, i % lines);
        }

    kDebug(5970) << desktopcount();
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

void EditTaskDialog::setTask( const QString &name, long time, long session )
{
    m_ui->tasknamelineedit->setText( name );
}

void EditTaskDialog::status(DesktopList *desktopList) const
{
    for ( int i = 0; i < 0; i++ )
    {
        //if ( _deskBox[i]->isChecked() ) desktopList->append( i );
    }
}

void EditTaskDialog::on_edittimespushbutton_clicked()
{
    historydialog* historydialog1=new historydialog(m_parent);
    lower();
    historydialog1->exec();
}
