#include "TitleBar.h"

#include <QHBoxLayout>
#include <QSpacerItem>

#include <QComboBox>
#include <QLabel>
#include <QStylePainter>
#include <QVariant>
#include <QStyleOptionToolBar>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    setProperty("panelwidget_multiple_row", true);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    setMaximumHeight(32);

    auto layout = new QHBoxLayout;
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(0);
    this->setLayout(layout);

    // just test
    auto combobox1 = new QComboBox(this);
    combobox1->addItem("Align Top");
    combobox1->addItem("Align Bootom");
    m_titleLabel = new QLabel("Title Bar", this);
    layout->addWidget(combobox1);
    layout->addWidget(m_titleLabel);

    connect(combobox1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
    {
        index == 0 ? setAlignment(Qt::AlignTop) : setAlignment(Qt::AlignBottom);
    });
}

void TitleBar::setMultipleRow(bool multiple)
{
    setProperty("panelwidget_multiple_row", multiple);
}

bool TitleBar::isMultipleRow() const
{
    return property("panelwidget_multiple_row").toBool();
}

void TitleBar::setAlignment(Qt::Alignment alignment)
{
    if(alignment == m_alignment)
        return;

    m_alignment = alignment;
    emit alignmentChanged(m_alignment);
}

void TitleBar::setTitle(const QString &title)
{
    if(title == m_titleLabel->text())
        return;

    m_titleLabel->setText(title);
    emit titleChanged(title);
}

Qt::Alignment TitleBar::alignment() const
{
    return m_alignment;
}

QString TitleBar::title() const
{
    return m_titleLabel->text();
}

void TitleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

//    QStylePainter painter(this);
//    QStyleOptionToolBar option;
//    option.rect = rect();
//    option.state = QStyle::State_Horizontal;
//    painter.drawControl(QStyle::CE_ToolBar, option);

    QPainter painter(this);
    QStyleOptionToolBar option;
    option.rect = rect();
    option.state = QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_ToolBar, &option, &painter, this);
}
