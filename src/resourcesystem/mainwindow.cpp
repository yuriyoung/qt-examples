#include "mainwindow.h"
#include "resourcewidget.h"

#include <QGridLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->setWindowTitle(tr("Resource System"));
    this->resize(640, 400);
    auto centralWidget = new QWidget();
    auto grid = new QGridLayout;

    auto widget = new ResourceWidget("D:\\sandboxol\\BlockmanEditor\\Media");
    widget->setNameFilters({"*.jpg", "*.jpeg", "*.png", "*.mesh", "*.mp3", "*.effect", "*.actor"});
    connect(widget, &ResourceWidget::rejected, this, &MainWindow::close);
    grid->addWidget(widget);
    centralWidget->setLayout(grid);
    this->setCentralWidget(centralWidget);
}

MainWindow::~MainWindow()
{
}

