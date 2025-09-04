#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    MainWindow window;
    window.setMinimumSize(1280, 720);
    window.show();
    return application.exec();
}
