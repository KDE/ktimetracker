#include <qprinter.h>
#include "karm.h"

class MyPrinter :public QPrinter
{
public:
  MyPrinter(const Karm *karm);
  void Print();
  void PrintLine(QString total, QString session, QString name, QPainter &);
  
  
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


