#include "widget.h"
#include "factory.h"
#include "singleton.h"
#include "builderpattern.h"
#include <QApplication>
#include <QGraphicsItem>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    return a.exec();
}
