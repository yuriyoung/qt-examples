#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "QToastWidget.h"

#include <QStyle>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->radioButton_topCenter->setChecked(true);
    ui->buttonGroup->setId(ui->radioButton_topLeft, 0);
    ui->buttonGroup->setId(ui->radioButton_topCenter, 1);
    ui->buttonGroup->setId(ui->radioButton_topRight, 2);
    ui->buttonGroup->setId(ui->radioButton_center, 3);
    ui->buttonGroup->setId(ui->radioButton_bottomLeft, 4);
    ui->buttonGroup->setId(ui->radioButton_bottomCenter, 5);
    ui->buttonGroup->setId(ui->radioButton_bottomRight, 6);

    connect(ui->pushButtonOnWindow, &QPushButton::clicked, this, &MainWindow::showToastInWindow);
    connect(ui->pushButtonOnScreen, &QPushButton::clicked, this, &MainWindow::showToastInScreen);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showToastInWindow()
{
    showToast(this);
}

void MainWindow::showToastInScreen()
{
    showToast();
}

void MainWindow::showToast(QWidget *parent)
{
    static int i = 0;
    const auto direction = QToastWidget::Direction(ui->buttonGroup->checkedId());
    QString message(QString("%1 Lorem ipsum dolor sit amet consectetur.").arg(++i));

    if(ui->radioButton_info->isChecked())
        QToastWidget::info(parent, message, direction);
    else if(ui->radioButton_success->isChecked())
        QToastWidget::success(parent, message, direction);
    else if(ui->radioButton_warning->isChecked())
        QToastWidget::warning(parent, message, direction);
    else if(ui->radioButton_error->isChecked())
        QToastWidget::error(parent, message, direction);
}

