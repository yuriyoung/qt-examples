#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "windowframehelper.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->dlgButton, &QPushButton::clicked, this, &MainWindow::showDialog);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showDialog()
{
    auto dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    WindowFrameHelper::makeWindowFrameless(dlg);
    dlg->resize(800, 600);
    dlg->exec();
}

