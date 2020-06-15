#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QImage>
#include <QColorSpace>
#include <QImageReader>
#include <QFileInfo>
#include <QScopeGuard>
#include <QMimeDatabase>
#include <QDebug>

class PreviewWidgetPrivate;
class PreviewWidget : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY(PreviewWidget)
    Q_DECLARE_PRIVATE_D(d, PreviewWidget)
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget();

    QString currentFile() const;
    void setCurrentFile(const QString &filename);

signals:
    void currentFileChanged(const QString &filename);

private:
    QScopedPointer<PreviewWidgetPrivate> d;
};

template<class BaseT>
class Previewer : public BaseT
{
public:
    enum Feature
    {
        Image,
        Audio,
        Video,
        Actor,
        Mesh,
        Effect
    };

    Previewer(QWidget *parent = nullptr) : BaseT(parent){}

    virtual bool load(const QString &filename) = 0;

    virtual int supportedFeature(const QString &filename) const
    {
        QMimeDatabase db;
        QMimeType mimetype = db.mimeTypeForFile(filename);
        QString name = mimetype.name();
        if(name.startsWith("image"))
            return Image;
        else if(name.startsWith("audio"))
            return Audio;
        else if(name.startsWith("video"))
            return Video;
        else if(name.compare("application-xml") == 0)
            return Actor;
        else
            return Mesh;
    }

//    virtual bool hasFeature() const = 0;

protected:
    void paintEvent(QPaintEvent *event)
    {
        if(m_availabled)
        {
            BaseT::paintEvent(event);
        }
        else
        {
            QPainter painter(this);
            painter.setPen(Qt::gray);
            painter.setFont(QFont("Arial", 10));
            painter.drawText(this->rect(), Qt::AlignCenter, "No preview");
        }
    }

    bool m_availabled = false;
};

/*
class PreviewerPrivate;
class Previewer : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(d, Previewer)
public:
    enum Feature
    {
        Image,
        Audio,
        Mesh,
        Effect
    };

    Previewer(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    QString currentFile() const;
    void setCurrentFile(const QString &filename);

    virtual int supportFeature() const = 0;
    virtual bool hasFeature() const = 0;

signals:
    void currentFileChanged(const QString &filename);

protected:
    Previewer(PreviewerPrivate &dd, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    QScopedPointer<PreviewerPrivate> d;
};
*/

class ImagePreviewer : public Previewer<QLabel>
{
    Q_OBJECT
public:
    ImagePreviewer(QWidget *parent = nullptr) : Previewer<QLabel>(parent)
    {
        setBackgroundRole(QPalette::Base);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setScaledContents(false);
        setAlignment(Qt::AlignCenter);
    }

    void zoomIn()
    {

    }

    void zoomOut()
    {

    }

    void scaleImage(double factor)
    {
        if(this->pixmap())
        {
            scaleFactor *= factor;
            resize(scaleFactor * this->pixmap()->size());
        }
    }

public slots:
    bool load(const QString &filename)
    {
        auto update = qScopeGuard([=]{ this->update(); });

        if(!QFileInfo(filename).isFile())
            return m_availabled = false;

        QImageReader reader(filename);
        reader.setAutoTransform(true);
        const QImage newImage = reader.read();
        if (newImage.isNull())
            return m_availabled = false;

        setImage(newImage);
        return m_availabled = true;
    }

private:
    void setImage(const QImage &newImage)
    {
        m_image = newImage;
        if (m_image.colorSpace().isValid())
            m_image.convertToColorSpace(QColorSpace::SRgb);

        setPixmap(QPixmap::fromImage(m_image));
        scaleFactor = 1.0;
    }

private:
    QImage m_image;
    double scaleFactor = 1;
};

class AudioPreviewer : public Previewer<QWidget>
{
    Q_OBJECT
public:
    AudioPreviewer(QWidget *parent = nullptr) : Previewer<QWidget>(parent) {}
};

class ActorPreviewer : public Previewer<QWidget>
{
    Q_OBJECT
public:
    ActorPreviewer(QWidget *parent = nullptr) : Previewer<QWidget>(parent) {}
};

#endif // PREVIEWWIDGET_H
