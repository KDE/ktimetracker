#ifndef ___print_h
#define ___print_h

#undef Color // X11 headers
#undef GrayScale // X11 headers
#include <qprinter.h>
#include <qpainter.h>
#include "karm.h"

class MyPrinter :public QPrinter
{
public:
  MyPrinter(const Karm *karm);
  void print();
  void printLine(QString total, QString session, QString name, QPainter &, int);
  void printTask(QListViewItem *item, QPainter &,int level);  
  int calculateReqNameWidth(QListViewItem *item, QFontMetrics &metrics, int level);
  
private:
  const Karm *_karm;

  int xMargin, yMargin;
  int yoff;
  int totalTimeWidth;
  int sessionTimeWidth;
  int nameFieldWidth;
  int lineHeight;
  int pageHeight;  
};

#endif

