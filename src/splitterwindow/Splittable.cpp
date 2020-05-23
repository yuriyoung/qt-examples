#include "Splittable.h"
#include "Splitter.h"
#include "Viewport.h"

#include <QSplitter>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QEvent>
#include <QRegion>
#include <QPoint>
#include <QPainter>
#include <QDebug>


/*
 * Overlay arrow:
 * (0,0)
 * +--------------------+
 * |                    |
 * |        | \         |
 * |        |  \        |
 * |--------+   \       |
 * |            /       | 1/4, 1/5
 * |--------+  /        |
 * |        | /         |
 * |        |/          |
 * |                    |
 * +--------------------+ (width, height)
 */
class Overlay : public QWidget
{
public:
    Overlay(Splittable *parent) : QWidget(parent)
    {
        this->resize(parent->width(), parent->height());
        this->raise();
        setStyleSheet("background-color: rgba(0, 0, 0, 127)");
    }

    void paintEvent(QPaintEvent */*event*/) override
    {
        QPainter painter(this);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#999999"));

        int offsetX = width() / 4;
        int offsetY = height() / 5;
        QPolygon basePoly;
        basePoly << QPoint(3 * offsetX, height() / 2)

                 << QPoint(2 * offsetX, 4 * offsetY)
                 << QPoint(2 * offsetX, 3 * offsetY)
                 << QPoint(0, 3 * offsetY)

                 << QPoint(0, 2 * offsetY)
                 << QPoint(2 * offsetX, 2 * offsetY)
                 << QPoint(2 * offsetX, offsetY);

        // TODO: translate direction with QMitrix

        painter.drawPolygon(basePoly);
    }

//private:
//    int direction = 0;
};

/*
 * default left-bottom corner mask:
 * 0,0
 * +---+  2, 0
 * |     *
 * |	   *
 * |	     *
 * |           *  12,10
 * | 		     |
 * +-------------+
 * 0,12           12,12
 *
 * other corner mask: ratate 90, 180, -90 with a QMatrix
 */

class SplittablePrivate
{
public:
    enum HintRegion
    {
        HintClient = 0,
        HintLeftTop,
        HintRightTop,
        HintLeftBottom,
        HintRightBottom
    };

    enum MoveDirection
    {
        MoveLeft = 0,// West
        MoveRight,  // East
        MoveUp,     // North
        MoveDown    // South
    };

    SplittablePrivate(Splittable *q) : q_ptr(q)
    {
        leftTopMask << QPoint(-2, -2)
                    << QPoint(14, -2)
                    << QPoint(12, 2)
                    << QPoint(2, 12)
                    << QPoint(-2, 14);
    }

    void reposition()
    {
        leftBottomMask = QMatrix().rotate(-90).map(leftTopMask);
        rightTopMask = QMatrix().rotate(90).map(leftTopMask);
        rightBottomMask = QMatrix().rotate(180).map(leftTopMask);

        int offsetX = q_ptr->width();
        int offsetY = q_ptr->height();
        leftBottomMask.translate(0, offsetY);
        rightTopMask.translate(offsetX, 0);
        rightBottomMask.translate(offsetX, offsetY);
    }

    int mouseMoveDirection(const QPoint start, const QPoint &end)
    {
        enum DirectionMask
        {
            Left    = 0x0001,
            Top     = 0x0010,
            Right   = 0x0100,
            Bottom  = 0x1000,
        };

        const auto direction =
                Left    * (end.x() < start.x()) |
                Right   * (end.x() > start.x()) |
                Top     * (end.y() < start.y()) |
                Bottom  * (end.y() > start.y());

        QPoint point = end - start;
        const int deltaX = qAbs(point.x());
        const int deltaY = qAbs(point.y());

        switch (direction)
        {
        case Left:          return MoveLeft;
        case Top:           return MoveUp;
        case Right:         return MoveRight;
        case Bottom:        return MoveDown;
        case Left | Top:    return deltaX >= deltaY ? MoveLeft  : MoveUp;   // default left if 45 degree
        case Left | Bottom: return deltaX >= deltaY ? MoveLeft  : MoveDown;
        case Right | Top:   return deltaX >= deltaY ? MoveRight : MoveUp;   // default right if 45 degree
        case Right | Bottom:return deltaX >= deltaY ? MoveRight : MoveDown;
        }

        return 0;
    }

    void holdUnsplit(Splittable *splitter)
    {
        // TODO: show this widget if splitter can be unsplited.
    }

    Splittable *q_ptr;
    QSplitter *splitter = nullptr;
    QStackedWidget *container = nullptr;
    QStackedLayout *layout = nullptr;
    QWidget *widget = nullptr;
    QWidget *overlay = nullptr;

    QPolygon leftTopMask; // as a base polygon
    QPolygon leftBottomMask;
    QPolygon rightTopMask;
    QPolygon rightBottomMask;

    HintRegion hintRegion = HintClient;
    QPoint directionOriginPos;
    bool splitting = false;
    bool inCorner = false;
};

/**
 * @brief Splittable::Splittable
 * @param widget
 */
Splittable::Splittable(QWidget *widget)
    : d(new SplittablePrivate(this))
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setAttribute(Qt::WA_Hover);

    d->layout = new QStackedLayout(this);
    d->layout->setSizeConstraint(QLayout::SetNoConstraint);

    d->widget = widget ? widget : new Viewport(this);
    d->layout->addWidget(d->widget);
}

Splittable::~Splittable()
{

}

void Splittable::split(Qt::Orientation orientation)
{
    Q_UNUSED(orientation)
}

void Splittable::split(Qt::Orientation orientation, int index)
{
    if(d->splitting)
        return;
    d->splitting = true;

    Q_ASSERT(d->splitter == nullptr);
    d->splitter = new Splitter(orientation, this);
    d->layout->addWidget(d->splitter);
    d->layout->removeWidget(d->widget);
    QWidget *originWidget = d->widget;
    d->widget = nullptr;

    // index only 0 and 1 while split two widget
    index = index >= 2 ? 1 : index;
    Splittable *duplicate = nullptr;
    Splittable *origin = nullptr;
    d->splitter->insertWidget(index, duplicate = new Splittable());
    d->splitter->insertWidget(!index, origin = new Splittable(originWidget));

    // set mini size for the newly
    QList<int> sizes = d->splitter->sizes();
    sizes[index] = 2;
    d->splitter->setSizes(sizes);

    d->layout->setCurrentWidget(d->splitter);

    d->splitter->handle(1)->grabMouse(QCursor(orientation == Qt::Horizontal
                                              ? Qt::SplitHCursor : Qt::SplitVCursor));
}

void Splittable::unsplit(bool all)
{
    Q_UNUSED(all)
    auto parentSplitter = qobject_cast<QSplitter *>(parentWidget());
    if(!parentSplitter)
        return;

    // TODO: IMPL

    QList<int> sizes = parentSplitter->sizes();
    int index = parentSplitter->indexOf(this);
    qDebug() << Q_FUNC_INFO << index << parentSplitter->count();
    if(index > 0 && index < parentSplitter->count())
    {
        sizes[index] += sizes[index - 1];
        sizes.removeAt(index - 1);
        delete parentSplitter->widget(index - 1);
    }

/*
    QWidget *widget = this->tabkeWidget();

    if(!d->splitter)
        return;

    Q_ASSERT(d->splitter->count() == 1);

    auto childSplittable = qobject_cast<Splittable *>(d->splitter->widget(0));
    QSplitter *origin = d->splitter;
    d->splitter = nullptr;
    if(childSplittable->hasSplitter())
    {
        d->splitter = childSplittable->takeSplitter();
        d->layout->addWidget(d->splitter);
        d->layout->setCurrentWidget(d->splitter);
    }
    else
    {
        d->widget = childSplittable->widget();
        d->layout->addWidget(d->widget);
        d->layout->setCurrentWidget(d->widget);
    }

    delete origin;
*/
}

QWidget *Splittable::widget() const
{
    return d->widget;
}

QWidget *Splittable::tabkeWidget()
{
    QWidget *origin = d->widget;
    if(d->widget)
    {
        d->layout->removeWidget(d->widget);
    }

    d->widget = nullptr;
    return origin;
}

bool Splittable::hasSplitter() const
{
    return d->splitter != nullptr;
}

QSplitter *Splittable::takeSplitter()
{
    QSplitter *origin = d->splitter;
    if(d->splitter)
    {
        d->layout->removeWidget(d->splitter);
    }

    d->splitter = nullptr;
    return origin;
}

void Splittable::mousePressEvent(QMouseEvent *event)
{
    qDebug() << Q_FUNC_INFO << event->button() <<  d->directionOriginPos;
    if(event->button() == Qt::LeftButton)
    {
        d->splitting = false;
        if(d->inCorner)
        {
//            qDebug() << Q_FUNC_INFO << event->button() <<  d->directionOriginPos;
            d->directionOriginPos = event->globalPos();
            event->accept();
        }
    }
    else
    {
        releaseMouse();
        event->ignore();
    }
}

void Splittable::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        qDebug() << Q_FUNC_INFO << event->button() << d->overlay;
        if(d->overlay)
        {
            delete d->overlay;
            d->overlay = nullptr;
        }
    }
}

void Splittable::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton)
    {
//        qDebug() << Q_FUNC_INFO << event->buttons();
        const QPoint pos = event->globalPos();
        switch (d->hintRegion)
        {
        case SplittablePrivate::HintClient:
            break;
        case SplittablePrivate::HintLeftTop:
        {
            int direction = d->mouseMoveDirection(d->directionOriginPos, pos);
            switch (direction)
            {
            case SplittablePrivate::MoveLeft:
                // create a overlay widget for unsplit
                d->holdUnsplit(this);
                break;
            case SplittablePrivate::MoveRight:
                this->split(Qt::Horizontal, 0);
                break;
            case SplittablePrivate::MoveUp:
                // create a overlay widget for unsplit
                d->holdUnsplit(this);
                break;
            case SplittablePrivate::MoveDown:
                this->split(Qt::Vertical, 0);
                break;
            }
            break;
        }
        case SplittablePrivate::HintRightTop:
        {
            int direction = d->mouseMoveDirection(d->directionOriginPos, pos);
            if(direction == SplittablePrivate::MoveLeft)
                this->split(Qt::Horizontal, 1);
            else if (direction == SplittablePrivate::MoveDown)
                this->split(Qt::Vertical, 0);

            break;
        }
        case SplittablePrivate::HintLeftBottom:
        {
            int direction = d->mouseMoveDirection(d->directionOriginPos, pos);
            if(direction == SplittablePrivate::MoveRight)
                this->split(Qt::Horizontal, 0);
            else if (direction == SplittablePrivate::MoveUp)
                this->split(Qt::Vertical, 1);
            break;
        }
        case SplittablePrivate::HintRightBottom:
        {
            int direction = d->mouseMoveDirection(d->directionOriginPos, pos);
            if(direction == SplittablePrivate::MoveLeft)
                this->split(Qt::Horizontal, 1);
            else if (direction == SplittablePrivate::MoveUp)
                this->split(Qt::Vertical, 1);
            break;
        }
        }
    }

    QWidget::mouseMoveEvent(event);
}

void Splittable::resizeEvent(QResizeEvent *)
{
    d->reposition();
}

bool Splittable::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::HoverEnter:
        this->hoverEnter(static_cast<QHoverEvent *>(event));
        break;
    case QEvent::HoverMove:
        this->hoverMove(static_cast<QHoverEvent *>(event));
        break;
    case QEvent::HoverLeave:
        this->hoverLeave(static_cast<QHoverEvent *>(event));
        break;

    default: break;
    }

    return QWidget::event(event);
}

void Splittable::hoverEnter(QHoverEvent *event)
{
//    qDebug() << Q_FUNC_INFO << event->type();
}

void Splittable::hoverMove(QHoverEvent *event)
{
    const QPoint cursor = event->pos();

    if(d->leftTopMask.containsPoint(cursor, Qt::OddEvenFill))
    {
//        qDebug() << Q_FUNC_INFO << event->type() << cursor;
        d->inCorner = true;
//        this->setMask(d->leftTopMask);
        d->hintRegion = SplittablePrivate::HintLeftTop;
        this->setCursor(Qt::CrossCursor);
    }
    else if(d->leftBottomMask.containsPoint(cursor, Qt::OddEvenFill))
    {
        d->inCorner = true;
//        this->setMask(d->leftBottomMask);
        d->hintRegion = SplittablePrivate::HintLeftBottom;
        this->setCursor(Qt::CrossCursor);
    }
    else if(d->rightTopMask.containsPoint(cursor, Qt::OddEvenFill))
    {
        d->inCorner = true;
//        this->setMask(d->rightTopMask);
        d->hintRegion = SplittablePrivate::HintRightTop;
        this->setCursor(Qt::CrossCursor);
    }
    else if(d->rightBottomMask.containsPoint(cursor, Qt::OddEvenFill))
    {
        d->inCorner = true;
//        this->setMask(d->rightBottomMask);
        d->hintRegion = SplittablePrivate::HintRightBottom;
        this->setCursor(Qt::CrossCursor);
    }
    else
    {
        d->inCorner = false;
        d->hintRegion = SplittablePrivate::HintClient;
        this->setCursor(Qt::ArrowCursor);
    }
}

void Splittable::hoverLeave(QHoverEvent *event)
{
//    qDebug() << Q_FUNC_INFO << event->type();
//    this->clearMask();
}
