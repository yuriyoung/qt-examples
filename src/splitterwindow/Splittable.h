#ifndef SPLITTABLE_H
#define SPLITTABLE_H

#include <QWidget>

class QSplitter;

class SplittablePrivate;
class Splittable : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(d, Splittable)
public:
    explicit Splittable(QWidget *widget = nullptr);
    ~Splittable() override;

    void split(Qt::Orientation orientation);
    void split(Qt::Orientation orientation, int index);
    void unsplit(bool all = false);

    QWidget *widget() const;
    QWidget *tabkeWidget();

    bool hasSplitter() const;
    QSplitter *takeSplitter();

signals:

public slots:

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *) override;
    bool event(QEvent *event) override;

    virtual void hoverEnter(QHoverEvent *event);
    virtual void hoverMove(QHoverEvent *event);
    virtual void hoverLeave(QHoverEvent *event);

private:
    QScopedPointer<SplittablePrivate> d;
};

#endif // SPLITTABLE_H
