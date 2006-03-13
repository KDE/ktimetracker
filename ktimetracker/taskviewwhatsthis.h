//
// C++ Interface: taskviewwhatsthis
//
//
// Author: Thorsten Staerk <thorsten@staerk.de>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TASKVIEWWHATSTHIS_H
#define TASKVIEWWHATSTHIS_H

#include <q3whatsthis.h>
#include <k3listview.h>

/**
this is the karm-taskview-specific implementation of qwhatsthis

@author Thorsten Staerk
*/
class TaskViewWhatsThis : public Q3WhatsThis
{
public:
    TaskViewWhatsThis( QWidget* );
    ~TaskViewWhatsThis();

    QString text ( const QPoint & );

private:
    K3ListView* _listView;  // stores the associated listview for column widths
};
#endif
