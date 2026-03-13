#include "mainwindow.h"
#include <QApplication>
#include <QMetaType>
#include <QVector>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QVector<LedNode>>("QVector<LedNode>");
    MainWindow w;
    w.show();
    return a.exec();
}
