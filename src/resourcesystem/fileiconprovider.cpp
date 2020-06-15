#include "fileiconprovider.h"

#include <QApplication>
#include <QStyle>

class FileIconProviderPrivate
{
public:
    FileIconProviderPrivate(FileIconProvider *q)
        : q_ptr(q)
        , acceptedTypes{"jpeg", "jpg", "tiff", "png", "psd"}
    {

    }

    FileIconProvider *q_ptr;
    QStringList acceptedTypes;
    // Mapping of file suffix to icon.
    mutable QHash<QString, QIcon> suffixCache;
    mutable QHash<QString, QIcon> filenameCache;
};

FileIconProvider::FileIconProvider()
    : QFileIconProvider(), d(new FileIconProviderPrivate(this))
{

}

QIcon FileIconProvider::icon(const QFileInfo &fileInfo) const
{
    bool isDir = fileInfo.isDir();
    if(isDir)
        return QIcon(":/icons/folder.png");

    const QString filename = !isDir ? fileInfo.fileName() : QString();
    if (!filename.isEmpty())
    {
        auto it = d->filenameCache.constFind(filename);
        if (it != d->filenameCache.constEnd())
            return it.value();

        // TODO: change ...
        if(d->acceptedTypes.contains(fileInfo.suffix()))
        {
            QPixmap pix(64,64);
            pix.load(fileInfo.absoluteFilePath());
            QIcon icon(pix);

//                QImageReader reader(fileInfo.absoluteFilePath());
//                reader.setAutoTransform(true);
//                const QImage image = reader.read();
//                QIcon icon(QPixmap::fromImage(image));

            d->filenameCache.insert(filename, icon);
            return icon;
        }
    }

    const QString suffix = !isDir ? fileInfo.suffix() : QString();
    if (!suffix.isEmpty())
    {
        auto it = d->suffixCache.constFind(suffix);
        if (it != d->suffixCache.constEnd())
            return it.value();
    }

    // TODO: Get icon from OS (and cache it based on suffix!)
    // ...
    static const QIcon unknownFileIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    QIcon icon = isDir ? QFileIconProvider::icon(fileInfo) : unknownFileIcon;

    if(!isDir && !suffix.isEmpty())
        d->suffixCache.insert(suffix, icon);

    return icon;
}
