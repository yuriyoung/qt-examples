#ifndef WINDOWFRAMEHELPER_H
#define WINDOWFRAMEHELPER_H

class QWidget;
class QWindow;

namespace WindowFrameHelper
{
    void makeWindowFrameless(QWidget *window);
    void makeWindowFrameless(QWindow *window);
}

#endif // WINDOWFRAMEHELPER_H
