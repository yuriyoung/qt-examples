#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QWidget>
#include <QBoxLayout>

class QStackedWidget;
class QVBoxLayout;
class TitleBar;
class Splittable;

class Viewport : public QWidget
{
    Q_OBJECT
public:
    explicit Viewport(Splittable *splittable, QWidget *parent = nullptr);
    ~Viewport() override;

    TitleBar *titleBar() const;

    void setSplitter(Splittable *splitter);
    Splittable *splitter() const;

    QBoxLayout::Direction layoutDirection() const;
    void setLayoutDirection(QBoxLayout::Direction direction) const;

    // TODO: create abstract widget interface
    void addWidget(QWidget *widget);
    QWidget *widget() const;

    void setCurrentWidget(int index);
    void setCurrentWidget(QWidget *widget);

    Viewport *duplicate();

signals:

public slots:

private:
    Splittable *m_splittable = nullptr;
    TitleBar *m_titleBar = nullptr;
    QStackedWidget *m_container = nullptr;
    QVBoxLayout *m_layout = nullptr;
};

#endif // VIEWPORT_H
