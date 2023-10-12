/*!
 * \file QToastWidget.cpp
 *
 * \author 12319597@qq.com
 * \date 2023/10/12
 *
 */
#include "QToastWidget.h"

#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QtEvents>
#include <QDebug>

static int Spacing = 8;

using QToastWidgetList = QVector<QToastWidget*>;
Q_GLOBAL_STATIC(QToastWidgetList, gToastList)

static Qt::Alignment DirectionToAlignment(QToastWidget::Direction direction)
{
    Qt::Alignment alignment;
    switch (direction)
    {
    case QToastWidget::Center:
        alignment = Qt::AlignCenter;
        break;

    case QToastWidget::TopLeft:
        alignment = Qt::AlignTop | Qt::AlignLeft;
        break;

    case QToastWidget::TopCenter:
        alignment = Qt::AlignTop | Qt::AlignHCenter;
        break;

    case QToastWidget::TopRight:
        alignment = Qt::AlignTop | Qt::AlignRight;
        break;

    case QToastWidget::BottomLeft:
        alignment = Qt::AlignBottom | Qt::AlignLeft;
        break;

    case QToastWidget::BottomCenter:
        alignment = Qt::AlignBottom | Qt::AlignHCenter;
        break;

    case QToastWidget::BottomRight:
        alignment = Qt::AlignBottom | Qt::AlignRight;
        break;

    default:
        break;
    }

    return alignment;
}

class QToastWidgetPrivate
{
public:
    QToastWidgetPrivate();
    ~QToastWidgetPrivate();

    void setupUi(QWidget *parent);
    bool isTop() const;
    bool isBottom() const;
    int currentIndex() const;
    QRect parentGeometry() const;
    void slideAllAnimations();
    void slideOneAnimation();
    QRect alignedDirection(const QSize& size, const QRect &rect, int margin = 0);

    void onShow();
    void onClose();
    void repaint();

    QToastWidget *q;
    QPropertyAnimation slideAnimation;

    // TODO: use style painter instead of
    QLabel* iconLabel;
    QLabel* textLabel;

    QIcon icon;
    QString text;
    QColor textColor;
    QColor backgroundColor;

    QTimer* progressTimer; // Display a progress bar that counts down until the toast closes
    int duration;
    bool enableProgress;
    qreal opacity;

    QToastWidget::Direction direction;
};

QToastWidgetPrivate::QToastWidgetPrivate()
    : backgroundColor(QApplication::palette().color(QPalette::Window))
    , progressTimer(new QTimer)
    , duration(3000)
    , enableProgress(false)
    , opacity(1.0f)
    , direction(QToastWidget::Direction::TopCenter)
{

}

QToastWidgetPrivate::~QToastWidgetPrivate()
{
    progressTimer->stop();
    delete progressTimer;
}

void QToastWidgetPrivate::setupUi(QWidget *parent)
{    
    iconLabel = new QLabel(parent);
    iconLabel->setScaledContents(true);
    iconLabel->setMaximumSize(64, 64);

    textLabel = new QLabel(parent);

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, parent);
    layout->setContentsMargins(8, 16, 8, 16);
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
}

bool QToastWidgetPrivate::isTop() const
{
    return (int)direction < int(QToastWidget::Center);
}

bool QToastWidgetPrivate::isBottom() const
{
    return (int)direction >= int(QToastWidget::Center);
}

int QToastWidgetPrivate::currentIndex() const
{
    return gToastList->indexOf(q);
}

QRect QToastWidgetPrivate::parentGeometry() const
{
    bool isDesktop = (q->parentWidget() == nullptr);
    QRect geometry;
    if(isDesktop)
    {
        QScreen *screen = q->screen();
        if(auto window = QApplication::activeWindow())
            screen = window->screen();
        geometry = screen->availableGeometry();
    }
    else
    {
        geometry = q->parentWidget()->rect();
    }

    return geometry;
}

void QToastWidgetPrivate::slideAllAnimations()
{
    for (int i = 0; i < gToastList->size(); ++i)
    {
        auto toast = gToastList->at(i);
        toast->d->slideOneAnimation();
    }
}

void QToastWidgetPrivate::slideOneAnimation()
{
    if(slideAnimation.state() == QAbstractAnimation::Running)
        return;

    const int i = currentIndex();
    const int y = i * (q->height() + Spacing);

    QRect geometry = parentGeometry();
    QRect rc = alignedDirection(q->size(), geometry, Spacing);
    rc.translate(0, isTop() ? y : -y);

    QPoint from = q->pos();
    QPoint to = rc.topLeft();
    if(from == to)
        return;

    slideAnimation.setTargetObject(q);
    slideAnimation.setPropertyName("pos");
    slideAnimation.setStartValue(from);
    slideAnimation.setEndValue(to);
    slideAnimation.setDuration(320);
    slideAnimation.setEasingCurve(QEasingCurve::OutCubic);
    slideAnimation.start(QAbstractAnimation::KeepWhenStopped);
}

QRect QToastWidgetPrivate::alignedDirection(const QSize &size, const QRect &rect, int margin)
{
    Qt::Alignment align = DirectionToAlignment(direction);
    auto r = rect.marginsRemoved(QMargins(margin, margin, margin, margin));
    return QStyle::alignedRect(QApplication::layoutDirection(), align, size, r);
}

void QToastWidgetPrivate::onShow()
{
    q->adjustSize();
    QRect rect =  alignedDirection(q->size(), parentGeometry());
    q->setGeometry(rect);

    gToastList->prepend(q);
    slideAllAnimations();
    progressTimer->start(duration);
}

void QToastWidgetPrivate::onClose()
{
    gToastList->removeOne(q);
    progressTimer->stop();
    slideAllAnimations();
}

void QToastWidgetPrivate::repaint()
{
    q->repaint();
    QCoreApplication::processEvents();
}


/**
 * @brief QToastWidget::QToastWidget
 * @param parent
 */
QToastWidget::QToastWidget(QWidget *parent)
    : QFrame(parent, Qt::FramelessWindowHint | (parent ? Qt::Widget : Qt::Window | Qt::WindowStaysOnTopHint))
    , d(new QToastWidgetPrivate())
{
    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    d->q = this;
    d->setupUi(this);

    connect(d->progressTimer, &QTimer::timeout, this, &QToastWidget::fadeOut);
}

QToastWidget::~QToastWidget()
{
    qDebug() << Q_FUNC_INFO;
}

QIcon QToastWidget::icon() const
{
    return d->icon;
}

void QToastWidget::setIcon(const QIcon &icon)
{
    if(icon.cacheKey() == d->icon.cacheKey())
        return;
    d->icon = icon;
    d->iconLabel->setPixmap(icon.pixmap(d->iconLabel->size()));
    Q_EMIT iconChanged();
}

QString QToastWidget::text() const
{
    return d->text;
}

void QToastWidget::setText(const QString &text)
{
    if(text == d->text)
        return;
    d->text = text;
    d->textLabel->setText(text);
    Q_EMIT textChanged();
}

bool QToastWidget::isProgressVisible() const
{
    return d->enableProgress;
}

void QToastWidget::setProgressVisible(bool visible)
{
    if(visible == d->enableProgress)
        return;
    d->enableProgress = visible;
}

int QToastWidget::duration() const
{
    return d->duration;
}

void QToastWidget::setDuration(int duration)
{
    if(duration == d->duration)
        return;
    d->duration = duration;
    Q_EMIT durationChanged();
}

QToastWidget::Direction QToastWidget::direction() const
{
    return d->direction;
}

void QToastWidget::setDirection(Direction direction)
{
    if(direction == d->direction)
        return;
    d->direction = direction;
    Q_EMIT directionChanged();
}

QColor QToastWidget::backgroundColor() const
{
    return d->backgroundColor;
}

void QToastWidget::setBackgroundColor(const QColor &newBackgroundColor)
{
    if (d->backgroundColor == newBackgroundColor)
        return;
    d->backgroundColor = newBackgroundColor;
    emit backgroundColorChanged();
}

qreal QToastWidget::opacity() const
{
    return d->backgroundColor.alphaF();
}

void QToastWidget::setOpacity(qreal opacity)
{
    if(qFuzzyCompare(d->opacity, opacity))
        return;

    d->opacity = opacity;
    if(!parentWidget())
    {
        setWindowOpacity(opacity);
    }
    else
    {
        this->update();
    }

    emit opacityChanged();
}

QSize QToastWidget::sizeHint() const
{
    return QFrame::sizeHint();
}

void QToastWidget::normal(QWidget *parent, const QString &text, const QIcon &icon, Direction direction)
{
    QToastWidget* toast = new QToastWidget(parent);
    toast->setDirection(direction);
    toast->setIcon(icon);
    toast->setText(text);
    toast->show();
}

void QToastWidget::info(QWidget *parent, const QString &text, Direction direction)
{
    QToastWidget::normal(parent, text, QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), direction);
}

void QToastWidget::success(QWidget *parent, const QString &text, Direction direction)
{
    QToastWidget* toast = new QToastWidget(parent);
    toast->setDirection(direction);
    toast->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton));
    toast->setText(text);
    toast->show();
}

void QToastWidget::warning(QWidget *parent, const QString &text, Direction direction)
{
    QToastWidget* toast = new QToastWidget(parent);
    toast->setDirection(direction);
    toast->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning));
    toast->setText(text);
    toast->show();
}

void QToastWidget::error(QWidget *parent, const QString &text, Direction direction)
{
    QToastWidget* toast = new QToastWidget(parent);
    toast->setDirection(direction);
    toast->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));
    toast->setText(text);
    toast->show();
}

// Show with animation
// Unused
// Warning: Widget can only one QGraphicsEffect object
// TODO: define a QGraphicsShadowEffect class provider a shadow and a opacity property
void QToastWidget::fadeIn()
{
    if(parentWidget())
    {
        QGraphicsOpacityEffect *fadeEffect = new QGraphicsOpacityEffect(this);
        this->setGraphicsEffect(fadeEffect);

        QPropertyAnimation *fadeInAnimation = new QPropertyAnimation(fadeEffect, "opacity");
        fadeInAnimation->setTargetObject(this);
        fadeInAnimation->setStartValue(0.f);
        fadeInAnimation->setEndValue(1.f);
        fadeInAnimation->setDuration(400);
        fadeInAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else
    {
        QPropertyAnimation *fadeOutAnimation = new QPropertyAnimation(this, "windowOpacity");
        fadeOutAnimation->setStartValue(0.f);
        fadeOutAnimation->setEndValue(1.f);
        fadeOutAnimation->setDuration(400);
        fadeOutAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    }
}

// Hide with animation
void QToastWidget::fadeOut()
{
    QPropertyAnimation *fadeOutAnimation = new QPropertyAnimation(this, "opacity");
    connect(fadeOutAnimation, &QAbstractAnimation::finished, this, &QToastWidget::close);
    fadeOutAnimation->setStartValue(1.f);
    fadeOutAnimation->setEndValue(0.f);
    fadeOutAnimation->setDuration(400);
    fadeOutAnimation->start(QPropertyAnimation::DeleteWhenStopped);
}

void QToastWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setOpacity(d->opacity);
    painter.setLayoutDirection(layoutDirection());
    drawContents(&painter);

    QFrame::paintEvent(event);
}

void QToastWidget::enterEvent(QEvent *event)
{
    d->progressTimer->stop();
    d->backgroundColor.setAlphaF(1.0f);
    QFrame::enterEvent(event);
}

void QToastWidget::leaveEvent(QEvent *event)
{
    d->progressTimer->start();
    QFrame::leaveEvent(event);
}

void QToastWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->close();
}

void QToastWidget::closeEvent(QCloseEvent *event)
{
    d->onClose();
    QFrame::closeEvent(event);
}

void QToastWidget::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    d->onShow();
}

void QToastWidget::drawContents(QPainter *painter)
{
    // draw border
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);
    QColor borderColor(palette().window().color().darker(150));
    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(d->backgroundColor);
    static QMargins margins = QMargins(1, 1, 1, 1);
    painter->drawRoundedRect(rect().marginsRemoved(margins), 4, 4);
    painter->restore();

    // TODO: draw elements use QStyleOption and QStylePainter instead of QLayout
    /*
    QStyleOption opt;
    opt.initFrom(this);

    QRect cr = contentsRect();
    cr.adjust(d->margin, d->margin, -d->margin, -d->margin);

    QStyle *style = QWidget::style();
    int flags = (layoutDirection() == Qt::LeftToRight
                         ? Qt::TextForceLeftToRight
                         : Qt::TextForceRightToLeft);
    style->drawItemText(painter, rect(), flags, opt.palette, isEnabled(), d->text, foregroundRole());
    */
}
