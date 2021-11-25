#include "mainwindow.h"
#include "propertybrowser.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_browser = new PropertyBrowser;
    setCentralWidget(m_browser);
}

MainWindow::~MainWindow()
{
}

