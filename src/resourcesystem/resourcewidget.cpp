#include "resourcewidget.h"
#include "resourcewidget_p.h"
#include "fileiconprovider.h"

#include <QGuiApplication>
#include <QMessageBox>
#include <QUrl>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QFileDialog>
#include <QDirIterator>
#include <QSortFilterProxyModel>
#include <QDebug>

Q_GLOBAL_STATIC(QUrl, lastVisitedDir)
static const qint32 ResourceWidgetMagic = 0xbe;


/**
 * Qt 5.15.0 below dose not support moveToTrash
 * Qt 5.15.0 or later has
 * see: https://bugreports.qt.io/browse/QTBUG-47703
 */
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

inline static void move_to_trash(const QString &file)
{
#ifdef Q_OS_WIN
    QFileInfo fileinfo(file);
    if(!fileinfo.exists())
        // TODO: change this
        throw std::exception( "File doesnt exists, cant move to trash" );

    WCHAR from[MAX_PATH];
    memset(from, 0, sizeof(from));
    int l = fileinfo.absoluteFilePath().toWCharArray(from);
    Q_ASSERT( 0 <= l && l < MAX_PATH );
    from[l] = '\0';
    SHFILEOPSTRUCT fileop;
    memset(&fileop, 0, sizeof(fileop));
    fileop.wFunc = FO_DELETE;
    fileop.pFrom = from;
    fileop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    int rv = SHFileOperation(&fileop);
    if(0 != rv)
    {
        qDebug() << rv << QString::number(rv).toInt(0, 8);
        // TODO: change this
        throw std::exception("move to trash failed");
    }
#elif Q_OS_LINUX
    // TODO: IMPL
#elif Q_OS_OSX
    // TODO: IMPL
#endif
}

inline static QUrl get_directory(const QUrl &url)
{
    if (url.isLocalFile())
    {
        QFileInfo info = QFileInfo(QDir::current(), url.toLocalFile());
        if (info.exists() && info.isDir())
            return QUrl::fromLocalFile(QDir::cleanPath(info.absoluteFilePath()));
        info.setFile(info.absolutePath());
        if (info.exists() && info.isDir())
            return QUrl::fromLocalFile(info.absoluteFilePath());
        return QUrl();
    }

    return url;
}

static QString name_filter_for_mime(const QString &mimeType)
{
    QMimeDatabase db;
    QMimeType mime(db.mimeTypeForName(mimeType));
    if (mime.isValid())
    {
        if (mime.isDefault())
        {
            return ResourceWidget::tr("All files (*)");
        }
        else
        {
            const QString patterns = mime.globPatterns().join(QLatin1Char(' '));
            return mime.comment() + QLatin1String(" (") + patterns + QLatin1Char(')');
        }
    }
    return QString();
}


/**
 * ********************************************************************************
 * @brief UiResourceWidget UI code
 * ********************************************************************************
 */
void UiResourceWidget::setupUi(QWidget *resourceWidget)
{
    if(resourceWidget->objectName().isEmpty())
        resourceWidget->setObjectName(QString::fromUtf8("ResourceWidget"));

    mainLayout = new QVBoxLayout(resourceWidget);

    // header
    tabbar = new QTabBar(resourceWidget);
    tabbar->setExpanding(false);
    tabbar->addTab(QObject::tr("Images"));
    tabbar->addTab(QObject::tr("Actors"));
    tabbar->addTab(QObject::tr("Items"));
    tabbar->addTab(QObject::tr("Audios"));
    tabbar->addTab(QObject::tr("Effects"));
    mainLayout->addWidget(tabbar);

    // content view
    splitter = new QSplitter(resourceWidget);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
    splitter->setSizePolicy(sizePolicy);
    splitter->setOrientation(Qt::Horizontal);

    dirView = new DirTreeView(splitter);
    dirView->setObjectName(QString::fromUtf8("TreeView"));
    splitter->addWidget(dirView);

    frame = new QFrame(splitter);
    frame->setObjectName(QString::fromUtf8("Frame"));
    frame->setFrameShape(QFrame::NoFrame);
    frame->setFrameShadow(QFrame::Raised);

    /* tool buttons for view */
    toolLayout = new QHBoxLayout();
    toolLayout->addWidget(searchEdit = new QLineEdit(resourceWidget));
    toolLayout->addWidget(backwardButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(forwardButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(addResourceButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(newFolderButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(removeButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(listModeButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(detailModeButton = new QToolButton(resourceWidget));
    toolLayout->addWidget(helpButton = new QToolButton(resourceWidget));

    vboxLayout = new QVBoxLayout(frame);
    vboxLayout->setSpacing(0);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    vboxLayout->addLayout(toolLayout);
    vboxLayout->addWidget(stackedWidget = new QStackedWidget(frame));

    /* set stacked widget pages and layouts */
    page1Layout = new QVBoxLayout(page1 = new QWidget());
    page1Layout->setSpacing(0);
    page1Layout->setContentsMargins(0, 0, 0, 0);
    page1Layout->addWidget(listView = new ResourceListView(resourceWidget));
    listView->setObjectName(QString::fromUtf8("ListView"));
    page2Layout = new QVBoxLayout(page2 = new QWidget());
    page2Layout->setSpacing(0);
    page2Layout->setContentsMargins(0, 0, 0, 0);
    page2Layout->addWidget(treeView = new ResourceTreeView(resourceWidget));
    treeView->setObjectName(QString::fromUtf8("TreeView"));
    stackedWidget->addWidget(page1);
    stackedWidget->addWidget(page2);
    stackedWidget->setCurrentIndex(0);

    /* resource preview */
    previewer = new PreviewWidget(splitter);
    previewer->setObjectName(QString::fromUtf8("Previewer"));

    mainLayout->addWidget(splitter);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 5);
    splitter->setStretchFactor(2, 3);

    // footer
    footerLayout = new QHBoxLayout();
    footerLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Preferred));
    footerLayout->addWidget(cancelButton = new QPushButton(resourceWidget));
    footerLayout->addWidget(noneButton = new QPushButton(resourceWidget));
    footerLayout->addWidget(okButton = new QPushButton(resourceWidget));
    mainLayout->addLayout(footerLayout);

    // set tab order
    QWidget::setTabOrder(tabbar, listView);
    QWidget::setTabOrder(listView, searchEdit);
    QWidget::setTabOrder(searchEdit, backwardButton);
    QWidget::setTabOrder(backwardButton, forwardButton);
    QWidget::setTabOrder(forwardButton, addResourceButton);
    QWidget::setTabOrder(addResourceButton, newFolderButton);
    QWidget::setTabOrder(addResourceButton, newFolderButton);
    QWidget::setTabOrder(newFolderButton, removeButton);
    QWidget::setTabOrder(removeButton, helpButton);
    QWidget::setTabOrder(helpButton, treeView);
    QWidget::setTabOrder(treeView, previewer);
    QWidget::setTabOrder(previewer, cancelButton);
    QWidget::setTabOrder(previewer, noneButton);
    QWidget::setTabOrder(noneButton, okButton);

    retranslateUi(resourceWidget);
}

void UiResourceWidget::retranslateUi(QWidget *resourceWidget)
{
    // TODO: translate tooltips
}

/**
 * ********************************************************************************
 * @brief ResourceOptionsPrivate
 * ********************************************************************************
 */
class ResourceOptionsPrivate : public QSharedData
{
public:
    ResourceOptionsPrivate() : options(0),
            viewMode(ResourceOptions::List),
            fileMode(ResourceOptions::AnyFile),
            acceptMode(ResourceOptions::AcceptOpen),
            filters(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs),
            useDefaultNameFilters(true)
        {}

    ResourceOptions::Options  options;
    ResourceOptions::ViewMode viewMode;
    ResourceOptions::FileMode fileMode;
    ResourceOptions::AcceptMode acceptMode;
    QDir::Filters filters;
    QList<QUrl> sidebarUrls;

    bool useDefaultNameFilters;
    QStringList nameFilters;
    QStringList mimeTypeFilters;
    QString defaultSuffix;
    QStringList history;

    QUrl initialDirectory;
    QString initiallySelectedMimeTypeFilter;
    QString initiallySelectedNameFilter;
    QList<QUrl> initiallySelectedFiles;
};

ResourceOptions::ResourceOptions(ResourceOptionsPrivate *dd) : d(dd)
{
}

ResourceOptions::~ResourceOptions()
{
}

namespace
{
    struct ResourceFileCombined : ResourceOptionsPrivate, ResourceOptions
    {
        ResourceFileCombined() : ResourceOptionsPrivate(), ResourceOptions(this) {}
        ResourceFileCombined(const ResourceFileCombined &other) : ResourceOptionsPrivate(other), ResourceOptions(this) {}
    };
}
QString ResourceOptions::defaultNameFilterString()
{
    return QCoreApplication::translate("ResourceSystem", "All Files (*)");
}

QSharedPointer<ResourceOptions> ResourceOptions::create()
{
    return QSharedPointer<ResourceFileCombined>::create();
}

QSharedPointer<ResourceOptions> ResourceOptions::clone() const
{
    return QSharedPointer<ResourceFileCombined>::create(*static_cast<const ResourceFileCombined*>(this));
}

void ResourceOptions::setOption(ResourceOptions::Option option, bool on)
{
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

bool ResourceOptions::testOption(ResourceOptions::Option option) const
{
    return d->options & option;
}

void ResourceOptions::setOptions(ResourceOptions::Options options)
{
    if (options != d->options)
        d->options = options;
}

ResourceOptions::Options ResourceOptions::options() const
{
    return d->options;
}

QDir::Filters ResourceOptions::filter() const
{
    return d->filters;
}

void ResourceOptions::setFilter(QDir::Filters filters)
{
    d->filters = filters;
}

void ResourceOptions::setViewMode(ResourceOptions::ViewMode mode)
{
    d->viewMode = mode;
}

ResourceOptions::ViewMode ResourceOptions::viewMode() const
{
    return d->viewMode;
}

void ResourceOptions::setFileMode(ResourceOptions::FileMode mode)
{
    d->fileMode = mode;
}

ResourceOptions::FileMode ResourceOptions::fileMode() const
{
    return d->fileMode;
}

void ResourceOptions::setAcceptMode(ResourceOptions::AcceptMode mode)
{
    d->acceptMode = mode;
}

ResourceOptions::AcceptMode ResourceOptions::acceptMode() const
{
    return d->acceptMode;
}

bool ResourceOptions::useDefaultNameFilters() const
{
    return d->useDefaultNameFilters;
}

void ResourceOptions::setUseDefaultNameFilters(bool dnf)
{
    d->useDefaultNameFilters = dnf;
}

void ResourceOptions::setNameFilters(const QStringList &filters)
{
    bool equal = filters.first() == ResourceOptions::defaultNameFilterString();
    d->useDefaultNameFilters = filters.size() == 1 && equal;
    d->nameFilters = filters;
}

QStringList ResourceOptions::nameFilters() const
{
    return d->useDefaultNameFilters ?
            QStringList(ResourceOptions::defaultNameFilterString()) : d->nameFilters;
}

void ResourceOptions::setMimeTypeFilters(const QStringList &filters)
{
    d->mimeTypeFilters = filters;
}

QStringList ResourceOptions::mimeTypeFilters() const
{
    return d->mimeTypeFilters;
}

void ResourceOptions::setDefaultSuffix(const QString &suffix)
{
    d->defaultSuffix = suffix;
    if (d->defaultSuffix.size() > 1 && d->defaultSuffix.startsWith(QLatin1Char('.')))
        d->defaultSuffix.remove(0, 1); // Silently change ".txt" -> "txt".
}

QString ResourceOptions::defaultSuffix() const
{
    return d->defaultSuffix;
}

void ResourceOptions::setHistory(const QStringList &paths)
{
    d->history = paths;
}

QStringList ResourceOptions::history() const
{
    return d->history;
}

QUrl ResourceOptions::initialDirectory() const
{
    return d->initialDirectory;
}

void ResourceOptions::setInitialDirectory(const QUrl &directory)
{
    d->initialDirectory = directory;
}

QString ResourceOptions::initiallySelectedMimeTypeFilter() const
{
    return d->initiallySelectedMimeTypeFilter;
}

void ResourceOptions::setInitiallySelectedMimeTypeFilter(const QString &filter)
{
    d->initiallySelectedMimeTypeFilter = filter;
}

QString ResourceOptions::initiallySelectedNameFilter() const
{
    return d->initiallySelectedNameFilter;
}

void ResourceOptions::setInitiallySelectedNameFilter(const QString &filter)
{
    d->initiallySelectedNameFilter = filter;
}

QList<QUrl> ResourceOptions::initiallySelectedFiles() const
{
    return d->initiallySelectedFiles;
}

void ResourceOptions::setInitiallySelectedFiles(const QList<QUrl> &files)
{
    d->initiallySelectedFiles = files;
}

/**
 * ********************************************************************************
 * @brief ResourceWidgetPrivate
 * ********************************************************************************
 */

ResourceWidgetPrivate::ResourceWidgetPrivate(ResourceWidget *q)
    : q_ptr(q)
    , ui(0)
    , options(ResourceOptions::create())
{

}

void ResourceWidgetPrivate::initialize(const QUrl &directory, const QString &nameFilter)
{
    Q_Q(ResourceWidget);
    createWidgets();

    if (!nameFilter.isEmpty())
        q->setNameFilter(nameFilter);

    const bool dontStoreDir = !directory.isValid() && !lastVisitedDir()->isValid();
    q->setDirectoryUrl(workingDirectory(directory));
    ui->dirView->setRootIndex(fsModel->index(directory.toLocalFile()));

    if(dontStoreDir)
        lastVisitedDir()->clear();

    if (directory.isLocalFile())
        q->selectFile(initialSelection(directory));
    else
        q->selectUrl(directory);
}

void ResourceWidgetPrivate::createWidgets()
{
    // do not recreate ui
    if(this->ui)
        return;

    Q_Q(ResourceWidget);
    ui.reset(new UiResourceWidget);
    ui->setupUi(q);

    // selections
//    QItemSelectionModel *selections = ui->listView->selectionModel();

    // init file navigation views
    createViews();
    // init tool buttons and menu actions for navigation views
    createViewToolButtons();
    createViewMenuActions();
    // translate ui lang
    retranslateStrings();

    // Initial widget states from options
    q->setViewMode(static_cast<ResourceWidget::ViewMode>(options->viewMode()));
    q->setOptions(static_cast<ResourceWidget::Options>(static_cast<int>(options->options())));
    q->setDirectoryUrl(options->initialDirectory());

    if (!options->mimeTypeFilters().isEmpty())
        q->setMimeTypeFilters(options->mimeTypeFilters());
    else if (!options->nameFilters().isEmpty())
        q->setNameFilters(options->nameFilters());

    q->selectNameFilter(options->initiallySelectedNameFilter());
    q->setDefaultSuffix(options->defaultSuffix());
    q->setHistory(options->history());
    const auto initiallySelectedFiles = options->initiallySelectedFiles();
    if (initiallySelectedFiles.size() == 1)
        q->selectFile(initiallySelectedFiles.first().fileName());
    for (const QUrl &url : initiallySelectedFiles)
        q->selectUrl(url);

    QObject::connect(ui->cancelButton, &QPushButton::released, q, &ResourceWidget::rejected);
    QObject::connect(ui->okButton, &QPushButton::released, q, &ResourceWidget::accepted);
}

void ResourceWidgetPrivate::createViews()
{
    Q_Q(ResourceWidget);

    fsModel = new QFileSystemModel(q);
    fsModel->setFilter(options->filter());
    fsModel->setNameFilterDisables(false);
    fsModel->setReadOnly(false);
    fsModel->setIconProvider(new FileIconProvider());
    ResourceWidget::connect(fsModel, &QFileSystemModel::fileRenamed, q,
                            [=](const QString &a, const QString &b, const QString &c){ onFileRenamed(a, b, c); });
    ResourceWidget::connect(fsModel, &QFileSystemModel::rootPathChanged,
                            q, [=](const QString &path) { onPathChanged(path); });
    ResourceWidget::connect(fsModel, &QFileSystemModel::rowsInserted,
                            q, [=](const QModelIndex &index) { onRowsInserted(index); });

    // directory view
    ui->dirView->setModel(fsModel);
    QObject::connect(ui->dirView, &QTreeView::clicked, q, [=](const QModelIndex &index)
    {
        onEnterDirectory(ui->dirView->mapToSource(index));
    });

    // file views
    ui->listView->setResourcePrivate(this);
    ui->listView->setModel(fsModel);
    QObject::connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged,
                     q, [=](const QItemSelection &current, const QItemSelection &/*previous*/)
    {
        if(!current.indexes().isEmpty())
            ui->previewer->setCurrentFile(fsModel->fileInfo(current.indexes().first()).absoluteFilePath());
    });
    QObject::connect(ui->listView, &QListView::activated, q, [=](const QModelIndex &index)
    {
        onEnterDirectory(index);
        ui->dirView->scrollTo(ui->dirView->mapFromSource(index), QAbstractItemView::PositionAtCenter);
    });
    QObject::connect(ui->listView, &QListView::customContextMenuRequested, q, [=](const QPoint &point)
    {
        onShowContextMenu(point);
    });

    ui->treeView->setResourcePrivate(this);
    ui->treeView->setModel(fsModel);
    QHeaderView *treeHeader = ui->treeView->header();
    QFontMetrics fm(q->font());
    treeHeader->resizeSection(0, fm.horizontalAdvance(QLatin1String("wwwwwwwwwwwwwwwwwwwwwwwwww")));
    treeHeader->resizeSection(1, fm.horizontalAdvance(QLatin1String("128.88 GB")));
    treeHeader->resizeSection(2, fm.horizontalAdvance(QLatin1String("mp3Folder")));
    treeHeader->resizeSection(3, fm.horizontalAdvance(QLatin1String("12/29/99 02:02PM")));
    treeHeader->setContextMenuPolicy(Qt::ActionsContextMenu);
    QObject::connect(ui->treeView, &QListView::activated, [=](const QModelIndex &index)
    {
        onEnterDirectory(index);
        ui->dirView->scrollTo(index, QAbstractItemView::PositionAtCenter);
    });
    QObject::connect(ui->treeView, &QListView::customContextMenuRequested, [=](const QPoint &point)
    {
        onShowContextMenu(point);
    });

    // previewer
    ui->previewer->setFrameShape(QFrame::Box);
    ui->previewer->setStyleSheet("#Previewer{border: 1px solid gray}");
}

void ResourceWidgetPrivate::createViewToolButtons()
{
    Q_Q(ResourceWidget);

    // search editor
    ui->searchEdit->setInputMethodHints(Qt::ImhNoPredictiveText);
    ui->searchEdit->setClearButtonEnabled(true);
    ui->searchEdit->addAction(QIcon(":/icons/search.png"), QLineEdit::LeadingPosition);
    QObject::connect(ui->searchEdit, &QLineEdit::textChanged, [=](const QString &text)
    {
        onSearch(text);
    });

    ui->backwardButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowBack, 0, q));
    ui->backwardButton->setAutoRaise(true);
    QObject::connect(ui->backwardButton, &QToolButton::clicked, [=]
    {
        onNavigateBackward();
    });

    ui->forwardButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowForward, 0, q));
    ui->forwardButton->setAutoRaise(true);
    QObject::connect(ui->forwardButton, &QToolButton::clicked, [=]
    {
        onNavigateForward();
    });

    ui->addResourceButton->setIcon(q->style()->standardIcon(QStyle::SP_DriveNetIcon, 0, q));
    ui->addResourceButton->setAutoRaise(true);
    QObject::connect(ui->addResourceButton, &QToolButton::clicked, [=]
    {
        onAddResource();
    });

    ui->newFolderButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogNewFolder, 0, q));
    ui->newFolderButton->setAutoRaise(true);
    QObject::connect(ui->newFolderButton, &QToolButton::clicked, [=]
    {
        onNewFolder();
    });

    ui->removeButton->setIcon(q->style()->standardIcon(QStyle::SP_TrashIcon, 0, q));
    ui->removeButton->setAutoRaise(true);
    QObject::connect(ui->removeButton, &QToolButton::clicked, [=]
    {
        onDeleteCurrent();
    });

    ui->listModeButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogListView, 0, q));
    ui->listModeButton->setAutoRaise(true);
    ui->listModeButton->setDown(true);
    QObject::connect(ui->listModeButton, &QToolButton::clicked, [=]
    {
        q->setViewMode(ResourceWidget::List);
    });

    ui->detailModeButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogDetailedView, 0, q));
    ui->detailModeButton->setAutoRaise(true);
    ui->detailModeButton->setDown(false);
    QObject::connect(ui->detailModeButton, &QToolButton::clicked, [=]
    {
        q->setViewMode(ResourceWidget::Detail);
    });

    ui->helpButton->setIcon(q->style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, q));
    ui->helpButton->setAutoRaise(true);
    QObject::connect(ui->helpButton, &QToolButton::clicked, [=]
    {
        // TODO: show help tooltips
    });
}

void ResourceWidgetPrivate::createViewMenuActions()
{
    Q_Q(ResourceWidget);

    copyAction = new QAction(q);
    copyAction->setEnabled(false);
    QObject::connect(copyAction, &QAction::triggered, [=]{ onCopyCurrent(); });

    cutAction = new QAction(q);
    cutAction->setEnabled(false);
    QObject::connect(cutAction, &QAction::triggered, [=]{ onCutCurrent(); });

    pasteAction = new QAction(q);
    pasteAction->setEnabled(false);
    QObject::connect(pasteAction, &QAction::triggered, [=]{ onPasteCurrent(); });

    deleteAction = new QAction(q);
    deleteAction->setEnabled(false);
    QObject::connect(deleteAction, &QAction::triggered, [=]{ onDeleteCurrent(); });

    renameAction = new QAction(q);
    renameAction->setEnabled(false);
    QObject::connect(renameAction, &QAction::triggered, [=]{ onRenameCurrent(); });

    addResourceAction = new QAction(q);
    addResourceAction->setEnabled(true);
    QObject::connect(addResourceAction, &QAction::triggered, [=]{ onAddResource(); });

    newFolderAction = new QAction(q);
    newFolderAction->setEnabled(true);
    QObject::connect(newFolderAction, &QAction::triggered, [=]{ onNewFolder(); });
}

void ResourceWidgetPrivate::retranslateStrings()
{
    ui->cancelButton->setText(ResourceWidget::tr("Cancel"));
    ui->noneButton->setText(ResourceWidget::tr("None Resource"));
    ui->okButton->setText(ResourceWidget::tr("OK"));

    copyAction->setText(ResourceWidget::tr("&Copy"));
    cutAction->setText(ResourceWidget::tr("Cu&t"));
    pasteAction->setText(ResourceWidget::tr("&Paste"));
    deleteAction->setText(ResourceWidget::tr("&Delete"));
    renameAction->setText(ResourceWidget::tr("&Rename"));
    addResourceAction->setText(ResourceWidget::tr("&Add Resource files"));
    newFolderAction->setText(ResourceWidget::tr("&New Folder"));
}

bool ResourceWidgetPrivate::itemViewKeyboardEvent(QKeyEvent *event)
{
#if QT_CONFIG(shortcut)
    Q_Q(ResourceWidget);
    if (event->matches(QKeySequence::Cancel))
    {
        q->reject();
        return true;
    }

#endif
    switch (event->key())
    {
    case Qt::Key_Backspace:
        onNavigateBackward();
        return true;
    case Qt::Key_Back:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplicationPrivate::keypadNavigationEnabled())
            return false;
#endif
    case Qt::Key_Left:
        if (event->key() == Qt::Key_Back || event->modifiers() == Qt::AltModifier)
        {
            onNavigateBackward();
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

void ResourceWidgetPrivate::setLastVisitedDirectory(const QUrl &dir)
{
    *lastVisitedDir() = dir;
}

QModelIndex ResourceWidgetPrivate::rootIndex() const
{
    return mapToSource(ui->listView->rootIndex());
}

void ResourceWidgetPrivate::setRootIndex(const QModelIndex &index)
{
    Q_ASSERT(index.isValid() ? index.model() == fsModel : true);
    QModelIndex idx = mapFromSource(index);
    ui->treeView->setRootIndex(idx);
    ui->listView->setRootIndex(idx);
}

QModelIndex ResourceWidgetPrivate::select(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid() ? index.model() == fsModel : true);

    QModelIndex idx = mapFromSource(index);
    if (idx.isValid() && !ui->listView->selectionModel()->isSelected(idx))
        ui->listView->selectionModel()->select(idx, QItemSelectionModel::Select
                                                    | QItemSelectionModel::Rows);

    return idx;
}

void ResourceWidgetPrivate::applyNameFilter()
{
    QStringList nameFilters = options->nameFilters();
    fsModel->setNameFilters(nameFilters);
}

QString ResourceWidgetPrivate::basename(const QString &path) const
{
    int sep = QDir::toNativeSeparators(path).lastIndexOf(QDir::separator());
    if (sep != -1)
        return path.mid(sep + 1);
    return path;
}

QString ResourceWidgetPrivate::environmentVariable(const QString &string)
{
#ifdef Q_OS_UNIX
    if (string.size() > 1 && string.startsWith(QLatin1Char('$')))
        return QString::fromLocal8Bit(qgetenv(string.midRef(1).toLatin1().constData()));
#else
    if (string.size() > 2 && string.startsWith(QLatin1Char('%')) && string.endsWith(QLatin1Char('%')))
        return QString::fromLocal8Bit(qgetenv(string.midRef(1, string.size() - 2).toLatin1().constData()));
#endif
    return string;
}

QUrl ResourceWidgetPrivate::workingDirectory(const QUrl &url)
{
    if (!url.isEmpty())
    {
        QUrl directory = get_directory(url);
        if (!directory.isEmpty())
            return directory;
    }

    QUrl directory = get_directory(*lastVisitedDir());
    if (!directory.isEmpty())
        return directory;

    return QUrl::fromLocalFile(QDir::currentPath());
}

QList<QUrl> ResourceWidgetPrivate::userSelectedFiles() const
{
    QList<QUrl> files;

    const QModelIndexList selectedRows = currentView()->selectionModel()->selectedRows();
    files.reserve(selectedRows.size());
    for (const QModelIndex &index : selectedRows)
        files.append(QUrl::fromLocalFile(index.data(QFileSystemModel::FilePathRole).toString()));

    return files;
}

QString ResourceWidgetPrivate::initialSelection(const QUrl &url)
{
    if (url.isEmpty())
        return QString();

    if (url.isLocalFile())
    {
        QFileInfo info(url.toLocalFile());
        if (!info.isDir())
            return info.fileName();
        else
            return QString();
    }

    return url.fileName();
}

void ResourceWidgetPrivate::saveHistorySelection()
{
    if (ui.isNull() || currentHistoryLocation < 0
            || currentHistoryLocation >= historyList.size())
        return;

    auto &item = historyList[currentHistoryLocation];
    item.selection.clear();
    const auto selectedIndexes = currentView()->selectionModel()->selectedRows();
    for (const auto &index : selectedIndexes)
        item.selection.append(QPersistentModelIndex(index));
}

void ResourceWidgetPrivate::navigate(ResourceWidgetPrivate::HistoryItem &historyItem)
{
    Q_Q(ResourceWidget);
    q->setDirectory(historyItem.path);
    // Restore selection unless something has changed in the file system
    if (ui.isNull() || historyItem.selection.isEmpty())
        return;

    if (std::any_of(historyItem.selection.cbegin(), historyItem.selection.cend(),
                    [](const QPersistentModelIndex &i) { return !i.isValid(); }))
    {
        historyItem.selection.clear();
        return;
    }

    QAbstractItemView *view = q->viewMode() == ResourceWidget::List
        ? static_cast<QAbstractItemView *>(ui->listView)
        : static_cast<QAbstractItemView *>(ui->treeView);
    auto selectionModel = view->selectionModel();
    const QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select
        | QItemSelectionModel::Rows;

    selectionModel->select(historyItem.selection.constFirst(),
                           flags | QItemSelectionModel::Clear | QItemSelectionModel::Current);
    for (int i = 1, size = historyItem.selection.size(); i < size; ++i)
        selectionModel->select(historyItem.selection.at(i), flags);

    view->scrollTo(historyItem.selection.constFirst());
}

QAbstractItemView *ResourceWidgetPrivate::currentView() const
{
    Q_Q(const ResourceWidget);

    QAbstractItemView *view = 0;
    if (q->viewMode() == ResourceWidget::Detail)
        view = ui->treeView;
    else
        view = ui->listView;

//    if (!ui->stackedWidget)
//        return 0;
//    if (ui->stackedWidget->currentWidget() == ui->listView->parent())
//        view = ui->listView;
//    else
//        view = ui->treeView;

    return view;
}

bool ResourceWidgetPrivate::restoreWidgetState(QStringList &history, int splitterPosition)
{
    Q_Q(ResourceWidget);
    if (splitterPosition >= 0)
    {
        QList<int> splitterSizes;
        splitterSizes.append(splitterPosition);
        splitterSizes.append(ui->splitter->widget(1)->sizeHint().width());
        ui->splitter->setSizes(splitterSizes);
    }
    else
    {
        if (!ui->splitter->restoreState(splitterState))
            return false;

        QList<int> list = ui->splitter->sizes();
        if (list.count() >= 2 && (list.at(0) == 0 || list.at(1) == 0))
        {
            for (int i = 0; i < list.count(); ++i)
                list[i] = ui->splitter->widget(i)->sizeHint().width();
            ui->splitter->setSizes(list);
        }
    }

    static const int MaxHistorySize = 5;
    if (history.size() > MaxHistorySize)
        history.erase(history.begin(), history.end() - MaxHistorySize);
    q->setHistory(history);

    QHeaderView *headerView = ui->treeView->header();
    if (!headerView->restoreState(headerData))
        return false;

    QList<QAction*> actions = headerView->actions();
    QAbstractItemModel *abstractModel = fsModel;
#if QT_CONFIG(proxymodel)
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    int total = qMin(abstractModel->columnCount(QModelIndex()), actions.count() + 1);
    for (int i = 1; i < total; ++i)
        actions.at(i - 1)->setChecked(!headerView->isSectionHidden(i));

    return true;
}

void ResourceWidgetPrivate::showDetailsView()
{
    ui->listModeButton->setDown(false);
    ui->detailModeButton->setDown(true);
    ui->listView->hide();
    ui->treeView->show();
    ui->stackedWidget->setCurrentWidget(ui->treeView->parentWidget());
    ui->treeView->doItemsLayout();
}

void ResourceWidgetPrivate::showListView()
{
    ui->listModeButton->setDown(true);
    ui->detailModeButton->setDown(false);
    ui->treeView->hide();
    ui->listView->show();
    ui->stackedWidget->setCurrentWidget(ui->listView->parentWidget());
    ui->listView->doItemsLayout();
}

// TODO: only navigate by current root directory
void ResourceWidgetPrivate::onNavigateToParent()
{
    Q_Q(ResourceWidget);
    QDir dir(fsModel->rootDirectory());
    QString newDirectory;
    if (dir.isRoot())
    {
        newDirectory = fsModel->myComputer().toString();
    }
    else
    {
        dir.cdUp();
        newDirectory = dir.absolutePath();
    }
    q->setDirectory(newDirectory);
    emit q->directoryEntered(newDirectory);
}

void ResourceWidgetPrivate::onNavigateForward()
{
    if (!historyList.isEmpty()
            && currentHistoryLocation < historyList.size() - 1)
    {
        saveHistorySelection();
        navigate(historyList[++currentHistoryLocation]);
    }
}

void ResourceWidgetPrivate::onNavigateBackward()
{
    if (!historyList.isEmpty() && currentHistoryLocation > 0)
    {
        saveHistorySelection();
        navigate(historyList[--currentHistoryLocation]);
    }
}

void ResourceWidgetPrivate::onGotoUrl(const QUrl &url)
{
    QModelIndex idx =  fsModel->index(url.toLocalFile());
    onEnterDirectory(idx);
}

void ResourceWidgetPrivate::onGotoDirectory(const QString &path)
{
    QModelIndex index = mapFromSource(fsModel->index(environmentVariable(path)));
    QDir dir(path);
    if (!dir.exists())
        dir.setPath(environmentVariable(path));

    if (dir.exists() || path.isEmpty() || path == fsModel->myComputer().toString())
    {
        onEnterDirectory(index);
    }
    else
    {
        QString message = ResourceWidget::tr("%1\nDirectory not found.\nPlease verify the "
                                                  "correct directory name was given.").arg(path);
        qWarning() << message;
    }
}

void ResourceWidgetPrivate::onEnterDirectory(const QModelIndex &index)
{
    Q_Q(ResourceWidget);

    QModelIndex sourceIndex = index.model() == proxyModel ? mapToSource(index) : index;
    QString path = sourceIndex.data(QFileSystemModel::FilePathRole).toString();
    if (path.isEmpty() || fsModel->isDir(sourceIndex))
    {
        q->setDirectory(path);
        emit q->directoryEntered(path);
    }
    else
    {
        // Do not accept when shift-clicking to multi-select a file in environments with single-click-activation
        if (!q->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, currentView())
            || !(QGuiApplication::keyboardModifiers() & Qt::CTRL))
        {
            q->accept();
        }
    }
}

void ResourceWidgetPrivate::onShowContextMenu(const QPoint &position)
{
    Q_Q(ResourceWidget);
    QAbstractItemView *view = 0;
    if (q->viewMode() == ResourceWidget::Detail)
        view = ui->treeView;
    else
        view = ui->listView;

    QModelIndex index = view->indexAt(position);
    index = mapToSource(index.sibling(index.row(), 0));

    QMenu menu(view);
    if (index.isValid())
    {
        // file context menu
        const bool ro = fsModel && fsModel->isReadOnly();
        QFile::Permissions p(index.parent().data(QFileSystemModel::FilePermissions).toInt());

        copyAction->setEnabled(!ro && p & QFile::ReadUser);
        menu.addAction(copyAction);

        cutAction->setEnabled(!ro && p & QFile::WriteUser);
        menu.addAction(cutAction);

        // TODO: set disabled if readonly or not permission or empty clipbord
        pasteAction->setEnabled(false);
        menu.addAction(pasteAction);

        deleteAction->setEnabled(!ro && p & QFile::WriteUser);
        menu.addAction(deleteAction);

        renameAction->setEnabled(!ro && p & QFile::WriteUser);
        menu.addAction(renameAction);

//        menu.addSeparator();
    }
    else
    {
        // TODO: set disabled if readonly or not permission or empty clipbord
        pasteAction->setEnabled(false);
        menu.addAction(pasteAction);

        addResourceAction->setEnabled(true);
        menu.addAction(addResourceAction);

        if (ui->newFolderButton->isVisible())
        {
            newFolderAction->setEnabled(ui->newFolderButton->isEnabled());
            menu.addAction(newFolderAction);
        }
    }

    if (ui->newFolderButton->isVisible())
    {
        newFolderAction->setEnabled(ui->newFolderButton->isEnabled());
        menu.addAction(newFolderAction);
    }

    menu.exec(view->viewport()->mapToGlobal(position));
}

void ResourceWidgetPrivate::onCurrentChanged(const QModelIndex &index)
{
    Q_Q(ResourceWidget);
    emit q->currentChanged(index.data(QFileSystemModel::FilePathRole).toString());
}

void ResourceWidgetPrivate::onPathChanged(const QString &newPath)
{
    const QModelIndex index = fsModel->index(newPath);
    ui->dirView->select(index);
    ui->searchEdit->setPlaceholderText(QString("Search\"%1\"").arg(QDir(newPath).dirName()));

    // back/forward history
    const QString newNativePath = QDir::toNativeSeparators(newPath);
    if (currentHistoryLocation < 0 || historyList.value(currentHistoryLocation).path != newNativePath)
    {
        if (currentHistoryLocation >= 0)
            saveHistorySelection();

        while (currentHistoryLocation >= 0 && currentHistoryLocation + 1 < historyList.count())
        {
            historyList.removeLast();
        }
        historyList.append({newNativePath, ModelIndexList()});
        ++currentHistoryLocation;
    }
}

/**
 * TODO: using a QSortFilterProxyModel
 * @brief ResourceWidgetPrivate::onSearch
 * @param text
 */
void ResourceWidgetPrivate::onSearch(const QString &text)
{
    qDebug() << text;
}

void ResourceWidgetPrivate::onCopyCurrent()
{
    // TODO:
}

void ResourceWidgetPrivate::onCutCurrent()
{
    // TODO:
}

void ResourceWidgetPrivate::onPasteCurrent()
{
    // TODO:
}

/**
 * @brief unused
 */
void ResourceWidgetPrivate::onRenameCurrent()
{
    QModelIndex index = currentView()->currentIndex();
    index = index.sibling(index.row(), 0);
    currentView()->edit(index);
}

/**
 * @brief Deletes the currently selected item in the resource view.
 *
 */
void ResourceWidgetPrivate::onDeleteCurrent()
{
    if (fsModel->isReadOnly())
        return;

    QModelIndexList list = currentView()->selectionModel()->selectedIndexes();
    for (int i = list.count() - 1; i >= 0; --i)
    {
        QPersistentModelIndex index = list.at(i);
        if (index == ui->listView->rootIndex())
            continue;

        index = mapToSource(index.sibling(index.row(), 0));
        if (!index.isValid())
            continue;

        QString fileName = index.data(QFileSystemModel::FileNameRole).toString();
        QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
        QFile::Permissions p(index.parent().data(QFileSystemModel::FilePermissions).toInt());

        // TODO: popup a warning confirm dialog for /*the protected*/ files/folders
//        if (!(p & QFile::WriteUser))
//            return;

        if (QMessageBox::warning(q_func(), QFileDialog::tr("Notice"),
                 QFileDialog::tr("Are you sure you want to delete '%1'?").arg(fileName),
                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
            return;

        if (fsModel->isDir(index) && !fsModel->fileInfo(index).isSymLink())
        {
            /* NOTE: Qt 5.15.0 below dose not support move file to trash */
//            QModelIndex modelIndex = fsModel->index(filePath);
//            fsModel->remove(modelIndex);

            move_to_trash(filePath);
        }
        else
        {
            /* NOTE: Qt 5.15.0 below dose not support move file to trash */
//            fsModel->remove(index);
            move_to_trash(fileName);
        }
    }
}

/**
 * @brief TODO: IMPLEMENT
 */
void ResourceWidgetPrivate::onAddResource()
{
    Q_Q(ResourceWidget);
    QStringList files = QFileDialog::getOpenFileNames(q->parentWidget());
    qDebug() << files;
}

void ResourceWidgetPrivate::onNewFolder()
{
    Q_Q(ResourceWidget);
    QAbstractItemView *view = currentView();
    view->clearSelection();

    QString newFolderString = ResourceWidget::tr("New Folder");
    QString folderName = newFolderString;
    QString prefix = q->directory().absolutePath() + QDir::separator();
    if (QFile::exists(prefix + folderName))
    {
        qlonglong suffix = 2;
        while (QFile::exists(prefix + folderName))
        {
            folderName = newFolderString + QString(" (%1)").arg(QString::number(suffix++));
        }
    }

    QModelIndex parent = rootIndex();
    QModelIndex index = fsModel->mkdir(parent, folderName);
    if (!index.isValid())
        return;

    index = select(index);
    if (index.isValid())
    {
        view->setCurrentIndex(index);
        view->edit(index);
    }
}

/**
 * ********************************************************************************
 * @brief ResourceWidget
 * ********************************************************************************
 */
ResourceWidget::ResourceWidget(QWidget *parent)
    : QWidget(parent)
    , d(new ResourceWidgetPrivate(this))
{
    d->initialize();
}

ResourceWidget::ResourceWidget(const QString &rootDirectory, QWidget *parent)
    : QWidget(parent)
    , d(new ResourceWidgetPrivate(this))
{
    d->initialize(QUrl::fromLocalFile(rootDirectory));
}

ResourceWidget::~ResourceWidget()
{

}

void ResourceWidget::setUserDirectory(const QString &directory)
{
    if(directory.compare(d->userDirectory, Qt::CaseInsensitive) == 0)
        return;

    d->userDirectory = directory;
    emit userDirectoryChanged(directory);
}

QString ResourceWidget::userDireectory() const
{
    return d->userDirectory;
}

void ResourceWidget::setDirectory(const QString &directory)
{
    QString newDirectory = directory;

    //remove .. and .
    if (!directory.isEmpty())
        newDirectory = QDir::cleanPath(directory);

    if (!directory.isEmpty() && newDirectory.isEmpty())
        return;

    QUrl newDirUrl = QUrl::fromLocalFile(newDirectory);
    d->setLastVisitedDirectory(newDirUrl);

    if (d->rootPath() == newDirectory)
        return;

    QModelIndex root = d->fsModel->setRootPath(newDirectory);
    if (root != d->rootIndex())
    {
        d->setRootIndex(root);
    }

    d->ui->listView->selectionModel()->clear();
}

void ResourceWidget::setDirectory(const QDir &directory)
{
    setDirectory(directory.absolutePath());
}

QDir ResourceWidget::directory() const
{
    Q_D(const ResourceWidget);
    return d->rootPath();
}

void ResourceWidget::setDirectoryUrl(const QUrl &directory)
{
    if (!directory.isValid())
            return;

    d->setLastVisitedDirectory(directory);

    if (directory.isLocalFile())
        setDirectory(directory.toLocalFile());
    else
        qWarning("Current supports only local files");
}

QUrl ResourceWidget::directoryUrl() const
{
    return QUrl::fromLocalFile(directory().absolutePath());
}

void ResourceWidget::selectFile(const QString &filename)
{
    if (filename.isEmpty())
        return;

    if (!QDir::isRelativePath(filename))
    {
        QFileInfo info(filename);
        QString filenamePath = info.absoluteDir().path();

        if (d->fsModel->rootPath() != filenamePath)
            setDirectory(filenamePath);
    }

    QModelIndex index = d->fsModel->index(filename);
    d->ui->listView->selectionModel()->clear();

    qDebug() << filename;
    // TODO: preview current selected file
    // index.isValid() ? index.data().toString() : fileFromPath(d->rootPath(), filename)
}

QStringList ResourceWidget::selectedFiles() const
{
    QStringList files;
    const QList<QUrl> userSelectedFiles = d->userSelectedFiles();
    files.reserve(userSelectedFiles.size());
    for (const QUrl &file : userSelectedFiles)
        files.append(file.toLocalFile());

    return files;
}

void ResourceWidget::selectUrl(const QUrl &url)
{
    if (!url.isValid())
        return;

    if (url.isLocalFile())
        selectFile(url.toLocalFile());
    else
        qWarning("Current supports only local files");
}

QList<QUrl> ResourceWidget::selectedUrls() const
{
    QList<QUrl> urls;
    const QStringList selectedFileList = selectedFiles();
    urls.reserve(selectedFileList.size());

    for (const QString &file : selectedFileList)
        urls.append(QUrl::fromLocalFile(file));

    return urls;
}

QDir::Filters ResourceWidget::filter() const
{
    return d->fsModel->filter();
//    return d->options->filter();
}

void ResourceWidget::setFilter(QDir::Filters filters)
{
    d->options->setFilter(filters);
    d->fsModel->setFilter(filters);
}

void ResourceWidget::setNameFilter(const QString &filter)
{
    QString f(filter);
    if (f.isEmpty())
        return;

    QString sep(QLatin1String(";;"));
    int i = f.indexOf(sep, 0);
    if (i == -1)
    {
        if (f.indexOf(QLatin1Char('\n'), 0) != -1)
        {
            sep = QLatin1Char('\n');
            i = f.indexOf(sep, 0);
        }
    }

    setNameFilters(f.split(sep));
}

void ResourceWidget::setNameFilters(const QStringList &filters)
{
    QStringList cleanedFilters;
    const int numFilters = filters.count();
    cleanedFilters.reserve(numFilters);
    for (int i = 0; i < numFilters; ++i)
    {
        cleanedFilters << filters[i].simplified();
    }

    if (cleanedFilters.isEmpty())
        return;

    d->options->setNameFilters(cleanedFilters);
    d->applyNameFilter();
}

QStringList ResourceWidget::nameFilters() const
{
    return d->options->nameFilters();
}

void ResourceWidget::selectNameFilter(const QString &filter)
{
    d->options->setInitiallySelectedNameFilter(filter);

    // TODO: filter with tab button
    // ...
}

void ResourceWidget::setMimeTypeFilters(const QStringList &filters)
{
    QStringList nameFilters;
    for (const QString &mimeType : filters)
    {
        const QString text = name_filter_for_mime(mimeType);
        if (!text.isEmpty())
            nameFilters.append(text);
    }
    setNameFilters(nameFilters);
    d->options->setMimeTypeFilters(filters);
}

QStringList ResourceWidget::mimeTypeFilters() const
{
    return d->options->mimeTypeFilters();
}

void ResourceWidget::selectMimeTypeFilter(const QString &filter)
{
    d->options->setInitiallySelectedMimeTypeFilter(filter);

    const QString filterForMime = name_filter_for_mime(filter);
    if (!filterForMime.isEmpty())
    {
        selectNameFilter(filterForMime);
    }
}

void ResourceWidget::setViewMode(ResourceWidget::ViewMode mode)
{
    d->options->setViewMode(static_cast<ResourceOptions::ViewMode>(mode));
    mode == Detail ? d->showDetailsView() : d->showListView();
}

ResourceWidget::ViewMode ResourceWidget::viewMode() const
{
    // TODO: using native filesystem (platform filesystem)
//    return (d->ui->stackedWidget->currentWidget() == d->ui->listView->parent()
//            ? ResourceWidget::List
//            : ResourceWidget::Detail);

    return static_cast<ResourceWidget::ViewMode>(d->options->viewMode());
}

void ResourceWidget::setOption(ResourceWidget::Option option, bool on)
{
    const ResourceWidget::Options previousOptions = options();
    if (!(previousOptions & option) != !on)
        setOptions(previousOptions ^ option);
}

bool ResourceWidget::testOption(ResourceWidget::Option option) const
{
    return d->options->testOption(static_cast<ResourceOptions::Option>(option));
}

void ResourceWidget::setOptions(Options options)
{
    Options changed = (options ^ ResourceWidget::options());
    if (!changed)
        return;

    d->options->setOptions(ResourceOptions::Options(int(options)));
//    d->createWidgets();

    if (changed & DontResolveSymlinks)
        d->fsModel->setResolveSymlinks(!(options & DontResolveSymlinks));

    if (changed & ReadOnly)
    {
        bool ro = (options & ReadOnly);
        d->fsModel->setReadOnly(ro);
        d->ui->newFolderButton->setEnabled(!ro);

        // TODO: update button actions state
//            d->renameAction->setEnabled(!ro);
//            d->deleteAction->setEnabled(!ro);
    }

//    if (changed & DontUseCustomDirectoryIcons)
//    {
//        QFileIconProvider::Options providerOptions = iconProvider()->options();
//        providerOptions.setFlag(QFileIconProvider::DontUseCustomDirectoryIcons,
//                                options & DontUseCustomDirectoryIcons);
//        iconProvider()->setOptions(providerOptions);
//    }

    if (changed & HideNameFilterDetails)
        setNameFilters(d->options->nameFilters());

    if (changed & ShowDirsOnly)
        setFilter((options & ShowDirsOnly) ? filter() & ~QDir::Files : filter() | QDir::Files);
}

ResourceWidget::Options ResourceWidget::options() const
{
    return ResourceWidget::Options(int(d->options->options()));
}


QByteArray ResourceWidget::saveState() const
{
    int version = 4;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(ResourceWidgetMagic);
    stream << qint32(version);
    stream << d->ui->splitter->saveState();
    stream << history();
    stream << *lastVisitedDir();
    stream << d->ui->treeView->header()->saveState();
    stream << qint32(viewMode());

    return data;
}

bool ResourceWidget::restoreState(const QByteArray &state)
{
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    QStringList history;
    QUrl currentDirectory;
    qint32 marker;
    qint32 version;
    qint32 viewMode;
    stream >> marker;
    stream >> version;

    // the code below only supports versions 3 and 4
    if (marker != ResourceWidgetMagic || (version != 3 && version != 4))
        return false;

    stream  >> d->splitterState
            >> history;
    if (version == 3)
    {
            QString currentDirectoryString;
            stream >> currentDirectoryString;
            currentDirectory = QUrl::fromLocalFile(currentDirectoryString);
    }
    else
    {
        stream >> currentDirectory;
    }
    stream >> d->headerData
           >> viewMode;

    setDirectoryUrl(lastVisitedDir()->isEmpty() ? currentDirectory : *lastVisitedDir());
    setViewMode(static_cast<ResourceWidget::ViewMode>(viewMode));

    return d->restoreWidgetState(history, -1);
}

void ResourceWidget::setDefaultSuffix(const QString &suffix)
{
    d->options->setDefaultSuffix(suffix);
}

QString ResourceWidget::defaultSuffix() const
{
    return d->options->defaultSuffix();
}

void ResourceWidget::setHistory(const QStringList &paths)
{
    Q_UNUSED(paths)
    // TODO: store history directories
}

QStringList ResourceWidget::history() const
{
    // TODO: get history directories

    QString currentHistory = QDir::toNativeSeparators(d->rootIndex().data(QFileSystemModel::FilePathRole).toString());
    return {currentHistory};
}

void ResourceWidget::setProxyModel(QAbstractProxyModel *model)
{
    if ((!model && !d->proxyModel) || (model == d->proxyModel))
        return;

    QModelIndex index = d->rootIndex();
    if (model != nullptr)
    {
        model->setParent(this);
        d->proxyModel = model;
        model->setSourceModel(d->fsModel);
        d->ui->listView->setModel(d->proxyModel);
        d->ui->treeView->setModel(d->proxyModel);
    }
    else
    {
        d->proxyModel = nullptr;
        d->ui->listView->setModel(d->fsModel);
        d->ui->treeView->setModel(d->fsModel);
    }

    QScopedPointer<QItemSelectionModel> selModel(d->currentView()->selectionModel());
    d->currentView()->setSelectionModel(d->currentView()->selectionModel());

    d->setRootIndex(index);

    QItemSelectionModel *selections = d->currentView()->selectionModel();
    QObject::connect(selections, &QItemSelectionModel::selectionChanged, this,
        [=](const QItemSelection &current, const QItemSelection &/*previous*/)
    {
        qDebug() << current.indexes().count();
    });
    QObject::connect(selections, &QItemSelectionModel::currentChanged, this,
        [=](const QModelIndex &current, const QModelIndex &/*previous*/)
    {
        qDebug() << current.data();
    });

}

QAbstractProxyModel *ResourceWidget::proxyModel() const
{
    return d->proxyModel;
}

void ResourceWidget::reject()
{
    emit rejected();
}

void ResourceWidget::accept()
{
    emit accepted();
}


/**
 * ********************************************************************************
 * @brief ResourceListView / ResourceTreeView
 * ********************************************************************************
 */
void ResourceListView::setResourcePrivate(ResourceWidgetPrivate *d_pointer)
{
    d_ptr = d_pointer;

    setIconSize(QSize(128, 56));
    setGridSize(QSize(128, 96));
    setSpacing(4);
    setUniformItemSizes(true);
    setWrapping(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Snap);
    setContextMenuPolicy(Qt::CustomContextMenu);

#if QT_CONFIG(draganddrop)
    setDragDropMode(QAbstractItemView::InternalMove);
#endif
}

QSize ResourceListView::sizeHint() const
{
    int height = qMax(10, sizeHintForRow(0));
    return QSize(QListView::sizeHint().width() * 2, height * 30);
}

void ResourceListView::keyPressEvent(QKeyEvent *event)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QListView::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

    if (!d_ptr->itemViewKeyboardEvent(event))
        QListView::keyPressEvent(event);
    event->accept();
}

void ResourceTreeView::setResourcePrivate(ResourceWidgetPrivate *d_pointer)
{
    d_ptr = d_pointer;
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSortingEnabled(true);
    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);

#if QT_CONFIG(draganddrop)
    setDragDropMode(QAbstractItemView::InternalMove);
#endif
}

QSize ResourceTreeView::sizeHint() const
{
    int height = qMax(10, sizeHintForRow(0));
    QSize sizeHint = header()->sizeHint();
    return QSize(sizeHint.width() * 4, height * 30);
}

void ResourceTreeView::keyPressEvent(QKeyEvent *event)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional)
    {
        QTreeView::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

    if (!d_ptr->itemViewKeyboardEvent(event))
        QTreeView::keyPressEvent(event);
    event->accept();
}

/**
 * ********************************************************************************
 * @brief DirFilterProxyModel
 * ********************************************************************************
 */
DirFilterProxyModel::DirFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
}

void DirFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    fileSystemModel = qobject_cast<QFileSystemModel *>(sourceModel);
    QSortFilterProxyModel::setSourceModel(sourceModel);
}

bool DirFilterProxyModel::hasChildren(const QModelIndex &parent) const
{
    QModelIndex source_parent = mapToSource(parent);
    if(!sourceModel()->hasChildren())
        return false;

    bool has = true;
    int size = sourceModel()->rowCount(source_parent);
    for(int i = 0; i < size; ++i)
    {
        if(fileSystemModel->isDir(sourceModel()->index(i, 0, source_parent)))
            return true;

        has = false;
    }
    return QSortFilterProxyModel::hasChildren(parent) && has;
}

bool DirFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto fs = qobject_cast<QFileSystemModel *>(sourceModel());

    // TODO: other filters
    // ...

    return fs ? fs->isDir(index) : true;
}

/**
 * ********************************************************************************
 * @brief DirTreeView
 * ********************************************************************************
 */
DirTreeView::DirTreeView(QWidget *parent) : QTreeView(parent)
{
    proxyModel = new DirFilterProxyModel(this);
    this->setHeaderHidden(true);
    setEditTriggers(QAbstractItemView::SelectedClicked
                    | QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
#if QT_CONFIG(draganddrop)
    setDragDropMode(QAbstractItemView::InternalMove);
#endif

    setItemDelegate(new DirDelegate(this));
}

void DirTreeView::setResourcePrivate(ResourceWidgetPrivate *d_pointer)
{
    d_ptr = d_pointer;
}

void DirTreeView::setModel(QAbstractItemModel *model)
{
    proxyModel->setSourceModel(model);
    QTreeView::setModel(proxyModel);

    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));
}

void DirTreeView::setRootIndex(const QModelIndex &index)
{
    if(!proxyModel->sourceModel())
    {
        this->setModel(const_cast<QAbstractItemModel *>(index.model()));
    }
    const QModelIndex proxyIndex = proxyModel->mapFromSource(index);
    QTreeView::setRootIndex(proxyIndex);
}

QModelIndex DirTreeView::mapFromSource(const QModelIndex &source) const
{
    return proxyModel->mapFromSource(source);
}

QModelIndex DirTreeView::mapToSource(const QModelIndex &index) const
{
    return proxyModel->mapToSource(index);
}

void DirTreeView::select(const QModelIndex &index)
{
    selectionModel()->clear();
    const QModelIndex idx = proxyModel->mapFromSource(index);
    selectionModel()->select(idx, QItemSelectionModel::Select);
    this->expand(idx);
}

void DirTreeView::loadUserDirectory(const QString &directory)
{

}

void DirTreeView::removeEntry()
{

}

#if QT_CONFIG(menu)
void DirTreeView::showContextMenu(const QPoint &position)
{
    QList<QAction *> actions;
    if (indexAt(position).isValid())
    {
        QAction *action = new QAction(ResourceWidget::tr("Remove"), this);
        if (indexAt(position).data().toUrl().path().isEmpty())
            action->setEnabled(false);

        connect(action, SIGNAL(triggered()), this, SLOT(removeEntry()));
        actions.append(action);
    }

    if (actions.count() > 0)
        QMenu::exec(actions, mapToGlobal(position));
}
#endif
