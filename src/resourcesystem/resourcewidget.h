#ifndef RESOURCEWIDGET_H
#define RESOURCEWIDGET_H

#include <QWidget>
#include <QDir>
#include <QUrl>

class QAbstractProxyModel;
class QTreeView;
class ResourceOptions;

class ResourceWidgetPrivate;
class ResourceWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ResourceWidget)
    Q_DECLARE_PRIVATE_D(d, ResourceWidget)
public:
    enum ViewMode { Detail, List, Icon };
    Q_ENUM(ViewMode)

    enum Option
    {
        ShowDirsOnly                = 0x00000001,
        DontResolveSymlinks         = 0x00000002,
        DontConfirmOverwrite        = 0x00000004,
        ReadOnly                    = 0x00000008,
        HideNameFilterDetails       = 0x00000010,
    };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    explicit ResourceWidget(QWidget *parent = nullptr);
    explicit ResourceWidget(const QString &rootDirectory, QWidget *parent = nullptr);
    virtual ~ResourceWidget();

    void setUserDirectory(const QString &directory);
    QString userDireectory() const;

    void setDirectory(const QString &directory);
    void setDirectory(const QDir &directory);
    QDir directory() const;

    void setDirectoryUrl(const QUrl &directory);
    QUrl directoryUrl() const;

    void selectFile(const QString &filename);
    QStringList selectedFiles() const;

    void selectUrl(const QUrl &url);
    QList<QUrl> selectedUrls() const;

    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void setNameFilter(const QString &filter);
    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void selectNameFilter(const QString &filter);
    QString selectedMimeTypeFilter() const;
    QString selectedNameFilter() const;

    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const;
    void selectMimeTypeFilter(const QString &filter);

    void setSidebarUrls(const QList<QUrl> &urls);
    QList<QUrl> sidebarUrls() const;

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void setOption(Option option, bool on = true);
    bool testOption(Option option) const;
    void setOptions(Options options);
    Options options() const;

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    void setDefaultSuffix(const QString &suffix);
    QString defaultSuffix() const;

    void setHistory(const QStringList &paths);
    QStringList history() const;

    void setProxyModel(QAbstractProxyModel *model);
    QAbstractProxyModel *proxyModel() const;

signals:
    // TODO: remove rejected/accepted/noneResourceSelected
    void rejected();
    void accepted();
    void noneResourceSelected();

    void searchTextChanged(const QString &text);
    void fileSelected(const QString &file);
    void filesSelected(const QStringList &files);
    void directoryEntered(const QString &directory);
    void currentChanged(const QString &path);
    void userDirectoryChanged(const QString &directory);

public slots:
    void reject();
    void accept();

protected:
    QScopedPointer<ResourceWidgetPrivate> d;
};

class ResourceOptionsPrivate;
class ResourceOptions
{
    Q_GADGET
    Q_DISABLE_COPY(ResourceOptions)
public:
    enum ViewMode { Detail, List, Icon };
    Q_ENUM(ViewMode)

    enum FileMode { AnyFile, ExistingFile, Directory, ExistingFiles, DirectoryOnly };
    Q_ENUM(FileMode)

    enum AcceptMode { AcceptOpen, AcceptNone };
    Q_ENUM(AcceptMode)

    enum Option
    {
        ShowDirsOnly                = 0x00000001,
        DontResolveSymlinks         = 0x00000002,
        DontConfirmOverwrite        = 0x00000004,
        ReadOnly                    = 0x00000008,
        HideNameFilterDetails       = 0x00000010,
    };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    static QString defaultNameFilterString();
    static QSharedPointer<ResourceOptions> create();
    QSharedPointer<ResourceOptions> clone() const;

    void setOption(Option option, bool on = true);
    bool testOption(Option option) const;
    void setOptions(ResourceOptions::Options options);
    Options options() const;

    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void setFileMode(FileMode mode);
    FileMode fileMode() const;

    void setAcceptMode(AcceptMode mode);
    AcceptMode acceptMode() const;

    bool useDefaultNameFilters() const;
    void setUseDefaultNameFilters(bool dnf);

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setMimeTypeFilters(const QStringList &filters);
    QStringList mimeTypeFilters() const;

    void setDefaultSuffix(const QString &suffix);
    QString defaultSuffix() const;

    void setHistory(const QStringList &paths);
    QStringList history() const;

    QUrl initialDirectory() const;
    void setInitialDirectory(const QUrl &directory);

    QString initiallySelectedMimeTypeFilter() const;
    void setInitiallySelectedMimeTypeFilter(const QString &filter);

    QString initiallySelectedNameFilter() const;
    void setInitiallySelectedNameFilter(const QString &filter);

    QList<QUrl> initiallySelectedFiles() const;
    void setInitiallySelectedFiles(const QList<QUrl> &files);

protected:
    ResourceOptions(ResourceOptionsPrivate *dd);
    ~ResourceOptions();

private:
    ResourceOptionsPrivate *d;
};

#endif // RESOURCEWIDGET_H
