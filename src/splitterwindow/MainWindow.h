#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class SplitterWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    SplitterWidget *m_splitter = nullptr;
};
#endif // MAINWINDOW_H
