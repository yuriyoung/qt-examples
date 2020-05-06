#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QObject>
#include <QWidget>

class QLabel;
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    void setMultipleRow(bool multiple);
    bool isMultipleRow() const;

    Qt::Alignment alignment() const;
    QString title() const;

signals:
    void alignmentChanged(Qt::Alignment alignment);
    void titleChanged(const QString &title);

public slots:
    void setAlignment(Qt::Alignment alignment);
    void setTitle(const QString &title);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Qt::Alignment m_alignment = Qt::AlignTop;
    QLabel *m_titleLabel;
};

#endif // TITLEBAR_H
