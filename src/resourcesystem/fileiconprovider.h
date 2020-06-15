#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <QFileIconProvider>

class FileIconProviderPrivate;
class FileIconProvider : public QFileIconProvider
{
    Q_DECLARE_PRIVATE_D(d, FileIconProvider)
public:
    FileIconProvider();

    QIcon icon(const QFileInfo &fileInfo) const override;

private:
    QScopedPointer<FileIconProviderPrivate> d;
};

#endif // FILEICONPROVIDER_H
