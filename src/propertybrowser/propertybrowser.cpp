#include "propertybrowser.h"
#include "editordelegate.h"

#include <QtWidgets>
#include <QtDebug>

PropertyBrowser::PropertyBrowser(QWidget *parent) : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);

    m_treeWidget = new QTreeWidget;
    layout->addWidget(m_treeWidget);

    m_treeWidget->setMouseTracking(true);
    m_treeWidget->setItemDelegate(new EditorDelegate(m_treeWidget));
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setAllColumnsShowFocus(false);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
//    m_treeWidget->setSelectionMode(QAbstractItemView::NoSelection);

    for (int i = 0; i < 100; ++i)
    {
        auto item = new QTreeWidgetItem;
        item->setText(0, QString("Item %1").arg(i));
        item->setData(1, Qt::DisplayRole, i);
        m_treeWidget->addTopLevelItem(item);
        auto w = new IntegerEditor(m_treeWidget->viewport());
        w->ensurePolished();
        w->setRange(0, 999);
        w->setValue(i);
        w->hide();
        item->setData(1, Qt::UserRole, QVariant::fromValue(w));

        connect(w, &IntegerEditor::valueChanged, this, [item](int value){
            item->setData(1, Qt::DisplayRole, value);
        });
    }

    connect(m_treeWidget, &QTreeWidget::itemEntered, this, [this](QTreeWidgetItem *item, int column){
//        if(column == 1)
//        {
            QWidget *widget = item->data(1, Qt::UserRole).value<QWidget *>();
            if(widget)
            {
                if(m_hoverdWidget)
                {
                    if((QApplication::focusWidget() && (QApplication::focusWidget()->parentWidget() == m_hoverdWidget))
                            || m_hoverdWidget->hasFocus())
                    {
                        m_focusedWidget = m_hoverdWidget;
                    }
                    else
                    {
                        m_hoverdWidget->hide();
                        QByteArray n = m_hoverdWidget->metaObject()->userProperty().name();
                        qDebug() << "hide prev widget: " << m_hoverdWidget->property(n);
                    }
                }

                m_hoverdWidget = widget;
                m_hoverdWidget->show();
            }
            qDebug() << "Mouse Ener item: " << item->text(0);
//        }
        m_treeWidget->setCurrentItem(item);
    });

    connect(qApp, &QApplication::focusChanged, this, [](QWidget *old, QWidget *now){
        if(old && old->metaObject()->className() == QStringLiteral("IntegerEditor"))
        {
            QByteArray n = old->metaObject()->userProperty().name();
            qDebug() << "lose focuse" << old->property(n);
            old->hide();
        }
    });
}

IntegerEditor::IntegerEditor(QWidget *parent)
    : QWidget(parent)
{
    m_slider = new QSlider(Qt::Horizontal);
    m_box = new QSpinBox;
    auto layout = new QHBoxLayout;
    layout->addWidget(m_slider, 1);
    layout->addWidget(m_box);
    this->setLayout(layout);

//    this->setFocusProxy(m_box);
    m_box->setRange(0, 999);
    connect(m_box, QOverload<int>::of(&QSpinBox::valueChanged), this, &IntegerEditor::valueChanged);
    connect(m_box, QOverload<int>::of(&QSpinBox::valueChanged), m_slider, &QSlider::setValue);
    connect(m_slider, &QSlider::valueChanged, this, &IntegerEditor::setValue);
}

void IntegerEditor::setRange(int min, int max)
{
    m_box->setRange(min, max);
}

void IntegerEditor::setValue(int value)
{
    if(value == m_box->value())
        return;

    m_box->setValue(value);
}

int IntegerEditor::value() const
{
    return m_box->value();
}
