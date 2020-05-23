#include "Splitter.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>

#include <QDebug>

/**
 * @brief The SplitterHandle class
 */
class SplitterHandle : public QSplitterHandle
{
public:
    explicit SplitterHandle(Qt::Orientation orientation, QSplitter *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

SplitterHandle::SplitterHandle(Qt::Orientation orientation, QSplitter *parent)
    : QSplitterHandle(orientation, parent)
{
    setMask(QRegion(contentsRect()));
    setAttribute(Qt::WA_MouseNoMask, true);
}

void SplitterHandle::mouseReleaseEvent(QMouseEvent *event)
{
    QSplitterHandle::mouseReleaseEvent(event);
    releaseMouse();
    qDebug() << Q_FUNC_INFO << event->type();
}

void SplitterHandle::resizeEvent(QResizeEvent *event)
{
    orientation() == Qt::Horizontal ? setContentsMargins(2, 0, 2, 0)
                                    : setContentsMargins(0, 2, 0, 2);
    setMask(QRegion(this->contentsRect()));
    QSplitterHandle::resizeEvent(event);
}

void SplitterHandle::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const QColor color("#999999");
    painter.fillRect(event->rect(), color);
}

class SplitterPrivate
{
    Q_DECLARE_PUBLIC(Splitter)
public:
    SplitterPrivate(Splitter *q)  : q_ptr(q) {}

    Splitter *q_ptr;
    SplitterHandle *handle = nullptr;
};

/**
 **************************************************************
 * @brief Splitter::Splitter
 * @param parent
 **************************************************************
 */
Splitter::Splitter(QWidget *parent)
    : QSplitter(parent)
    , d(new SplitterPrivate(this))
{
    setObjectName("Splitter");
    setHandleWidth(1);
    setChildrenCollapsible(false);
}

Splitter::Splitter(Qt::Orientation orientation, QWidget *parent)
     : QSplitter(orientation, parent)
     , d(new SplitterPrivate(this))
{
    setObjectName("Splitter");
    setHandleWidth(1);
    setChildrenCollapsible(false);
}

Splitter::~Splitter()
{

}

QSplitterHandle *Splitter::createHandle()
{
    d->handle = new SplitterHandle(this->orientation(), this);
    return  d->handle;
}

void Splitter::paintEvent(QPaintEvent *event)
{
//    auto palette = this->palette();
//    palette.setColor(QPalette::Active, QPalette::Base, Qt::transparent);
//    palette.setColor(QPalette::Inactive, QPalette::Base, Qt::transparent);
    return QSplitter::paintEvent(event);
}
