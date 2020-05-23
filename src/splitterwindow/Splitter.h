#ifndef SPLITTER_H
#define SPLITTER_H

#include <QSplitter>

class SplitterPrivate;
class Splitter : public QSplitter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(d, Splitter)
public:
    explicit Splitter(QWidget *parent = nullptr);
    explicit Splitter(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~Splitter();

    inline int pickHandle(const QPoint &pos) const
    { return this->orientation() == Qt::Horizontal ? pos.x() : pos.y(); }

    inline int pickHandle(const QSize &size) const
    { return this->orientation() == Qt::Horizontal ? size.width() : size.height(); }

    inline int transHandle(const QPoint &pos) const
    { return this->orientation() == Qt::Vertical ? pos.x() : pos.y(); }

    inline int transHandle(const QSize &size) const
    { return this->orientation() == Qt::Vertical ? size.width() : size.height(); }

protected:
    QSplitterHandle *createHandle();
    void paintEvent(QPaintEvent *event);

private:
    QScopedPointer<SplitterPrivate> d;
};

#endif // SPLITTER_H
