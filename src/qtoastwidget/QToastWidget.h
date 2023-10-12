#ifndef QTOASTWIDGET_H
#define QTOASTWIDGET_H

#include <QFrame>
#include <QIcon>

class QToastWidgetPrivate;

/**
 * @brief The QToastWidget class
 *  Toastify
 */
class QToastWidget : public QFrame
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(d, QToastWidget)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
public:
    // Anchor Position
    enum Direction
    {
        TopLeft,
        TopCenter,
        TopRight,
        Center,
        BottomLeft,
        BottomCenter,
        BottomRight
    };
    Q_ENUM(Direction);

    // Unused
    // TODO: support options
    enum Option
    {
        AutoClose       = 0x0001, // Milliseconds to wait before closing toast.
        DelayOpen       = 0x0002, // Milliseconds to wait before opening toast.
        CloseOnClick    = 0x0004, // Closes toast when mouse left button click on it.
        OpenOnFocus     = 0x0008, // Activate the task when the task is focused.
        OpenOnHover     = 0x0010, // Activate the task when the task is hovered.
        ShowProgress    = 0x0020,
        DefaultOptions  = AutoClose | CloseOnClick | OpenOnFocus | OpenOnHover
    };

    Q_ENUM(Option);
    Q_DECLARE_FLAGS(Options, Option);

    explicit QToastWidget(QWidget *parent = nullptr);
    ~QToastWidget() override;

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    QString text() const;
    void setText(const QString& text);

    // Display a progress bar that counts down until the taost closes.
    bool isProgressVisible() const;
    void setProgressVisible(bool visible);

    // Time (in milliseconds) to wait until taost is automatically hidden.
    int duration() const;
    void setDuration(int duration);

    Direction direction() const;
    void setDirection(QToastWidget::Direction direction);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &newBackgroundColor);

    qreal opacity() const;
    void setOpacity(qreal opacity);

    QSize sizeHint() const override;

    static void normal(QWidget *parent, const QString& text, const QIcon& icon = QIcon(), Direction direction = TopCenter);
    static void info(QWidget *parent, const QString& text, Direction direction = TopCenter);
    static void success(QWidget *parent, const QString& text, Direction direction = TopCenter);
    static void warning(QWidget *parent, const QString& text, Direction direction = TopCenter);
    static void error(QWidget *parent, const QString& text, Direction direction = TopCenter);

signals:
    void iconChanged();
    void textChanged();
    void durationChanged();
    void directionChanged();
    void backgroundColorChanged();
    void opacityChanged();

public slots:
    virtual void fadeIn();
    virtual void fadeOut();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

    virtual void drawContents(QPainter *painter);

private:
    QScopedPointer<QToastWidgetPrivate> d;
};

#endif // QTOASTWIDGET_H
