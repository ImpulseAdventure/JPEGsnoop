#include <QApplication>
#include <QDebug>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("ImpulseAdventure");
    QCoreApplication::setOrganizationDomain("www.impulseadventure.com");
    QCoreApplication::setApplicationName("JPEGsnoopQt");

    MainWindow w;
    w.show();

    return a.exec();
}
