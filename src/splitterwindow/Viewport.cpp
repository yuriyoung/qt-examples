#include "Viewport.h"
#include "TitleBar.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QListView>
#include <QLabel>
#include <QDrag>
#include <QMimeData>
#include <QtMath>
#include <QDebug>

Viewport::Viewport(Splittable *splittable, QWidget *parent)
    : QWidget(parent)
    , m_splittable(splittable)
    , m_titleBar(new TitleBar(this))
    , m_container(new QStackedWidget(this))
{
    setObjectName("Viewport");

    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    {
        connect(m_titleBar, &TitleBar::alignmentChanged, this, [this] {
            if(m_titleBar->alignment() == Qt::AlignTop)
                this->setLayoutDirection(QBoxLayout::TopToBottom);
            else
                this->setLayoutDirection(QBoxLayout::BottomToTop);
        });
    }

    m_layout->addWidget(m_titleBar);
    m_layout->addWidget(m_container);

    m_container->setStyleSheet("background-color: #333333; color: #999999; font-size: 20px");
    QLabel *label = new QLabel("Splittable\n Viewport");
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_container->addWidget(label);
}

Viewport::~Viewport()
{

}

TitleBar *Viewport::titleBar() const
{
    return m_titleBar;
}

void Viewport::setSplitter(Splittable *splitter)
{
    m_splittable = splitter;
}

Splittable *Viewport::splitter() const
{
    return m_splittable;
}

QBoxLayout::Direction Viewport::layoutDirection() const
{
    return m_layout->direction();
}

void Viewport::setLayoutDirection(QBoxLayout::Direction direction) const
{
    m_layout->setDirection(direction);
}

void Viewport::addWidget(QWidget *widget)
{
    if((m_container->indexOf(widget)) != -1)
        return;

    m_container->addWidget(widget);
    m_container->setCurrentWidget(widget);
}

QWidget *Viewport::widget() const
{
    return m_container->currentWidget();
}

void Viewport::setCurrentWidget(int index)
{
    Q_ASSERT(index >= 0 && index < m_container->count());
    m_container->setCurrentIndex(index);
}

void Viewport::setCurrentWidget(QWidget *widget)
{
    Q_ASSERT(widget != nullptr);
    m_container->setCurrentWidget(widget);
}

Viewport *Viewport::duplicate()
{
    // TODO: IMPL
    return nullptr;
}
