#include "MainWindow.h"
#include "Splittable.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(480, 320);
//    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    Splittable *splittable = new Splittable();
    splittable->setParent(this);
    this->setCentralWidget(splittable);
}

MainWindow::~MainWindow()
{
}

