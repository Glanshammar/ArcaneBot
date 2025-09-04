#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ArcaneLibrary.h"
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_DriverButton_clicked()
{
    TestDriver();
}

