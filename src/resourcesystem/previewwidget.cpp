#include "previewwidget.h"

#include <QStackedLayout>

class PreviewWidgetPrivate
{
public:
    PreviewWidgetPrivate(PreviewWidget *q)
    {
        container = new QStackedLayout(q);
        container->setSizeConstraint(QLayout::SetNoConstraint);
        container->setContentsMargins(0, 0, 0, 0);

        auto ip = new ImagePreviewer(q);
        container->addWidget(ip);
        PreviewWidget::connect(q, &PreviewWidget::currentFileChanged, ip, &ImagePreviewer::load);
    }

    QStackedLayout *container = nullptr;
    QHash<int, QWidget *> previewers;
    QString currentFile;
};

PreviewWidget::PreviewWidget(QWidget *parent)
    : QFrame(parent), d(new PreviewWidgetPrivate(this))
{
    resize(200, parentWidget()->height());
}

PreviewWidget::~PreviewWidget()
{

}


QString PreviewWidget::currentFile() const
{
    return d->currentFile;
}

void PreviewWidget::setCurrentFile(const QString &filename)
{
    if(filename.compare(d->currentFile, Qt::CaseInsensitive) == 0)
        return;

    d->currentFile = filename;
    emit currentFileChanged(filename);
}


class PreviewerPrivate
{
public:
    PreviewerPrivate() {}
    virtual ~PreviewerPrivate() {}

    QString currentFile;
};

//Previewer::Previewer(QWidget *parent, Qt::WindowFlags f)
//    : Previewer(*new PreviewerPrivate, parent, f)
//{

//}

//Previewer::Previewer(PreviewerPrivate &dd, QWidget *parent, Qt::WindowFlags f)
//    : QWidget(parent, f), d(&dd)
//{

//}
