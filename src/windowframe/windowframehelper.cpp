#include "windowframehelper.h"

#include <QGuiApplication>
#include <QAbstractScrollArea>
#include <QWidget>
#include <QWindow>
#include <QScreen>
#include <QLayout>
#include <QtEvents>
#include <QPointer>
#include <QDebug>

class WindowFrame : public QObject
{
    Q_DISABLE_COPY(WindowFrame)
public:
    WindowFrame() = delete;
    explicit WindowFrame(QWidget *target);
    ~WindowFrame();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool event(QEvent *event) override;

    virtual void showEvent(QShowEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void moveEvent(QMoveEvent *event);
    virtual void mouseHoverEvent(QHoverEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    Qt::Edges edgesFromPoint(const QPoint &pos) const;
    void updateCursorShape(Qt::Edges edges);
    bool atBottom() const;
    bool atLeft() const;
    void updateGeometry(const QPoint &pos);

private:
    QPointer<QWidget> m_target;
    int m_dxMax;
    int m_dyMax;
    int m_border;

    bool m_resizeEnabled;
    bool m_moveEnabled;
    bool m_mousePressed;

    QPoint m_pressedPos;
    QRect m_rect;
    Qt::Edges m_edges;
};

void WindowFrameHelper::makeWindowFrameless(QWidget *window)
{
    if(!window)
        return;

    (void)new WindowFrame(window);
}

void WindowFrameHelper::makeWindowFrameless(QWindow *window)
{
    if(!window)
        return;

    // TODO: IMPLEMENT ME
}

WindowFrame::WindowFrame(QWidget *target)
    : QObject(target)
    , m_target(target)
    , m_dxMax(0)
    , m_dyMax(0)
    , m_border(5)
    , m_resizeEnabled(false)
    , m_moveEnabled(false)
    , m_mousePressed(false)
{
    Q_ASSERT(target != nullptr);

    const bool visibled = target->isVisible();

    // ensure it is a top level window
    auto flags = target->windowFlags();
    if(!flags.testFlag(Qt::Dialog) && !flags.testFlag(Qt::Window))
        flags |= Qt::Window;
    target->setWindowFlags(flags | Qt::FramelessWindowHint);

    target->setAttribute(Qt::WA_Hover);
    target->setVisible(visibled);
    target->installEventFilter(this);
}

WindowFrame::~WindowFrame()
{
    qDebug() << Q_FUNC_INFO;
}

bool WindowFrame::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::Show:
        showEvent(static_cast<QShowEvent*>(event));
        break;

    case QEvent::HoverMove:
        mouseHoverEvent(static_cast<QHoverEvent*>(event));
        break;

    case QEvent::MouseMove:
        mouseMoveEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent((QMouseEvent*)event);
        break;

    case QEvent::Enter:
        enterEvent(event);
        break;

    case QEvent::Leave:
        leaveEvent(event);
        break;

    case QEvent::Move:
        moveEvent((QMoveEvent*)event);
        break;

    case QEvent::Resize:
        resizeEvent((QResizeEvent*)event);
        break;

    default:
        break;
    }

    return m_target->eventFilter(watched, event);
}

bool WindowFrame::event(QEvent *event)
{
    return QObject::event(event);
}

void WindowFrame::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
}

void WindowFrame::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
}

void WindowFrame::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
}

void WindowFrame::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
}

void WindowFrame::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
    // We're inside a resize operation; no update necessary.
    if(m_mousePressed)
        return;
}

void WindowFrame::mouseHoverEvent(QHoverEvent *event)
{
    Qt::Edges edges = edgesFromPoint(event->pos());
    updateCursorShape(edges);
}

void WindowFrame::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() != Qt::LeftButton || m_resizeEnabled || m_moveEnabled)
        return;

    if (!m_mousePressed || m_target->testAttribute(Qt::WA_WState_ConfigPending))
        return;

    updateGeometry(event->globalPos());
}

void WindowFrame::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    m_pressedPos = event->globalPos();
    m_mousePressed = true;
    m_rect = m_target->geometry();

    if (m_target->isWindow()
            && m_target->windowHandle()
            && !(m_target->windowFlags() & Qt::X11BypassWindowManagerHint)
            && !m_target->testAttribute(Qt::WA_DontShowOnScreen)
            && !m_target->hasHeightForWidth())
    {
        m_edges = edgesFromPoint(m_target->mapFromGlobal(m_pressedPos));
        if(m_edges != 0)
        {
            m_resizeEnabled = m_target->windowHandle()->startSystemResize(m_edges);
        }
        else
        {
            m_moveEnabled = m_target->windowHandle()->startSystemMove();
        }
    }

    if(m_resizeEnabled || m_moveEnabled)
        return;

    // Find available desktop/workspace geometry.
    QRect availableGeometry;
    bool hasVerticalSizeConstraint = true;
    bool hasHorizontalSizeConstraint = true;
    if (m_target->isWindow())
    {
        availableGeometry =m_target-> windowHandle()->screen()->availableGeometry();
    }
    else
    {
        const QWidget *tlwParent = m_target->parentWidget();
        QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(tlwParent->parentWidget());
        if (scrollArea)
        {
            hasHorizontalSizeConstraint = scrollArea->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
            hasVerticalSizeConstraint = scrollArea->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
        }
        availableGeometry = tlwParent->contentsRect();
    }

    // TODO: Find frame geometries, title bar height, and decoration sizes
    // ...

    if (atBottom())
    {
        if (hasVerticalSizeConstraint)
            m_dyMax = availableGeometry.bottom() - m_rect.bottom() - m_border;
        else
            m_dyMax = INT_MAX;
    }
    else
    {
        if (hasVerticalSizeConstraint)
            m_dyMax = availableGeometry.y() - m_rect.y() + m_border; // TODO: + title bar height
        else
            m_dyMax = -INT_MAX;
    }

    if (atLeft())
    {
        if (hasHorizontalSizeConstraint)
            m_dxMax = availableGeometry.x() - m_rect.x() + m_border;
        else
            m_dxMax = -INT_MAX;
    }
    else
    {
        if (hasHorizontalSizeConstraint)
            m_dxMax = availableGeometry.right() - m_rect.right() - m_border;
        else
            m_dxMax = INT_MAX;
    }
}

void WindowFrame::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_mousePressed = false;
        m_pressedPos = QPoint();
    }
}

Qt::Edges WindowFrame::edgesFromPoint(const QPoint &pos) const
{
    Qt::Edges edges;
    if(pos.x() < m_border)
       edges |= Qt::LeftEdge;
    if(pos.y() < m_border)
       edges |= Qt::TopEdge;
    if(pos.x() > m_target->width() - m_border)
       edges |= Qt::RightEdge;
    if(pos.y() > m_target-> height() - m_border)
        edges |= Qt::BottomEdge;

    return edges;
}

void WindowFrame::updateCursorShape(Qt::Edges edges)
{
    Qt::CursorShape cursorShape;
    if (edges == Qt::LeftEdge || edges == Qt::RightEdge)
        cursorShape = Qt::SizeHorCursor;
    else if (edges == Qt::TopEdge || edges == Qt::BottomEdge)
        cursorShape = Qt::SizeVerCursor;
    else if (edges == (Qt::TopEdge | Qt::LeftEdge))
        cursorShape = Qt::SizeFDiagCursor;
    else if (edges == (Qt::TopEdge | Qt::RightEdge))
        cursorShape = Qt::SizeBDiagCursor;
    else if (edges == (Qt::BottomEdge | Qt::LeftEdge))
        cursorShape = Qt::SizeBDiagCursor;
    else if (edges == (Qt::BottomEdge | Qt::RightEdge))
        cursorShape = Qt::SizeFDiagCursor;
    else
        cursorShape = Qt::ArrowCursor;

    m_target->setCursor(cursorShape);
}

bool WindowFrame::atBottom() const
{
    return m_edges == (Qt::BottomEdge | Qt::LeftEdge) || m_edges == (Qt::BottomEdge | Qt::RightEdge);
}

bool WindowFrame::atLeft() const
{
    return m_edges == (Qt::BottomEdge | Qt::LeftEdge) || m_edges == (Qt::TopEdge | Qt::LeftEdge);
}

void WindowFrame::updateGeometry(const QPoint &pos)
{
    QSize ns;
    if(atBottom())
        ns.rheight() = m_rect.height() + qMin(pos.y() - m_pressedPos.y(), m_dyMax);
    else
        ns.rheight() = m_rect.height() - qMax(pos.y() - m_pressedPos.y(), m_dyMax);

    if (atLeft())
        ns.rwidth() = m_rect.width() - qMax(pos.x() - m_pressedPos.x(), m_dxMax);
    else
        ns.rwidth() = m_rect.width() + qMin(pos.x() - m_pressedPos.x(), m_dxMax);

    ns = QLayout::closestAcceptableSize(m_target, ns);
    QPoint p;
    QRect nr(p, ns);
    if (atBottom())
    {
        if (atLeft())
            nr.moveTopRight(m_rect.topRight());
        else
            nr.moveTopLeft(m_rect.topLeft());
    }
    else
    {
        if (atLeft())
            nr.moveBottomRight(m_rect.bottomRight());
        else
            nr.moveBottomLeft(m_rect.bottomLeft());
    }

    m_target->setGeometry(nr);
}
