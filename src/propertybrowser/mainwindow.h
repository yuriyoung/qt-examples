#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class PropertyBrowser;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    PropertyBrowser *m_browser;
};
#endif // MAINWINDOW_H
