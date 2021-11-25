#ifndef PROPERTYBROWSER_H
#define PROPERTYBROWSER_H

#include <QWidget>

class QTreeWidget;
class QSpinBox;
class QSlider;

class PropertyBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyBrowser(QWidget *parent = nullptr);

signals:

private:
    QTreeWidget *m_treeWidget;
    QWidget *m_hoverdWidget = nullptr;
    QWidget *m_focusedWidget = nullptr;
};

class IntegerEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)
public:
    explicit IntegerEditor(QWidget *parent);

    void setRange(int min, int max);

    void setValue(int value);
    int value() const;

signals:
    void valueChanged(int value);

private:
    QSpinBox *m_box;
    QSlider *m_slider;
};

#endif // PROPERTYBROWSER_H
