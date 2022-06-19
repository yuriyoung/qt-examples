#include "mainwindow.h"
#include "windowframehelper.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    WindowFrameHelper::makeWindowFrameless(&w);
    w.show();
	
	// or here
	// WindowFrameHelper::makeWindowFrameless(&w);

    return a.exec();
}
