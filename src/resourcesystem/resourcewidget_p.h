#ifndef RESOURCEWIDGET_P_H
#define RESOURCEWIDGET_P_H

#include "resourcewidget.h"
#include "previewwidget.h"

#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QPersistentModelIndex>
#include <QTabBar>
#include <QTreeView>
#include <QListView>
#include <QStackedWidget>
#include <QAbstractProxyModel>
#include <QSplitter>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QButtonGroup>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QUrl>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>
#include <QFileIconProvider>
#include <QApplication>
#include <QResizeEvent>
#include <QDebug>

class DirProxyModel;
class DirTreeView;
class ResourceListView;
class ResourceTreeView;

class UiResourceWidget
{
public:
    // main layout
//    QGridLayout *gridLayout;
    QVBoxLayout *mainLayout;

    // header tabbar
    QTabBar      *tabbar;

    // content
    // file view tool buttons
    QHBoxLayout *toolLayout;
    QLineEdit   *searchEdit;
    QToolButton *backwardButton;
    QToolButton *forwardButton;
    QToolButton *addResourceButton;
    QToolButton *newFolderButton;
    QToolButton *removeButton;
    QToolButton *listModeButton;
    QToolButton *detailModeButton;
    QToolButton *helpButton;

    // view splitter
    QSplitter   *splitter;
    /* left: sidebar/dir view */
    DirTreeView *dirView;
    /* center: file views */
    QFrame      *frame;
    QVBoxLayout *vboxLayout;
    QStackedWidget   *stackedWidget;
    QWidget     *page1;
    QVBoxLayout *page1Layout;
    QWidget     *page2;
    QVBoxLayout *page2Layout;
    ResourceListView *listView;
    ResourceTreeView *treeView;
    /* right: preview */
    PreviewWidget *previewer;

    // footer action buttons
    QHBoxLayout *footerLayout;
    QPushButton *cancelButton;
    QPushButton *noneButton;
    QPushButton *okButton;

    void setupUi(QWidget *resourceWidget);

private:
    void retranslateUi(QWidget *resourceWidget);
};

class ResourceWidgetPrivate
{
    Q_DECLARE_PUBLIC(ResourceWidget)
public:
    using ModelIndexList = QVector<QPersistentModelIndex>;
    struct HistoryItem
    {
        QString path;
        ModelIndexList selection;
    };

    ResourceWidgetPrivate(ResourceWidget *q);

    static void setLastVisitedDirectory(const QUrl &dir);

    void initialize(const QUrl &directory = QUrl(), const QString &nameFilter = QString());
    void createWidgets();
    void createViews();
    void createViewToolButtons();
    void createViewMenuActions();
    void retranslateStrings();

    bool itemViewKeyboardEvent(QKeyEvent *event);

    QModelIndex rootIndex() const;
    inline QModelIndex mapToSource(const QModelIndex &index) const;
    inline QModelIndex mapFromSource(const QModelIndex &index) const;
    inline QString rootPath() const;
    void setRootIndex(const QModelIndex &index);
    QModelIndex select(const QModelIndex &index) const;

    void applyNameFilter();
    QString basename(const QString &path) const;
    QString environmentVariable(const QString &string);
    QUrl workingDirectory(const QUrl &url);
    QList<QUrl> userSelectedFiles() const;
    QString initialSelection(const QUrl &url);
    void saveHistorySelection();
    void navigate(HistoryItem &historyItem);
    QAbstractItemView *currentView() const;
    bool restoreWidgetState(QStringList &history, int splitterPosition);
    void showDetailsView();
    void showListView();

    // private slots
    void onNavigateToParent();
    void onNavigateForward();
    void onNavigateBackward();
    void onGotoUrl(const QUrl &url);
    void onGotoDirectory(const QString &path);
    void onEnterDirectory(const QModelIndex &index);
    void onShowContextMenu(const QPoint &position);
    void onSelectionChanged();
    void onCurrentChanged(const QModelIndex &index);

    void onFileRenamed(const QString &/*path*/, const QString &/*oldName*/, const QString &/*newName*/) { }
    void onPathChanged(const QString &newPath);
    void onRowsInserted(const QModelIndex &/*parent*/) {}
    void onSearch(const QString &text);

    void onCopyCurrent();
    void onCutCurrent();
    void onPasteCurrent();
    void onRenameCurrent();
    void onDeleteCurrent();
    void onAddResource();
    void onNewFolder();

    /**
     * @brief data members
     */
    ResourceWidget *q_ptr = nullptr;
    int currentHistoryLocation = -1;

    QFileSystemModel *fsModel = nullptr;
    QAbstractProxyModel *proxyModel = nullptr;

    QScopedPointer<UiResourceWidget> ui;
    QSharedPointer<ResourceOptions> options;

    QString userDirectory;

    QAction *copyAction = nullptr;
    QAction *cutAction = nullptr;
    QAction *pasteAction = nullptr;
    QAction *deleteAction = nullptr;
    QAction *renameAction = nullptr;
    QAction *addResourceAction = nullptr;
    QAction *newFolderAction = nullptr;

    QList<HistoryItem> historyList;
    QByteArray splitterState;
    QByteArray headerData;
};

QModelIndex ResourceWidgetPrivate::mapToSource(const QModelIndex &index) const
{
    return proxyModel ? proxyModel->mapToSource(index) : index;
}

QModelIndex ResourceWidgetPrivate::mapFromSource(const QModelIndex &index) const
{
    return proxyModel ? proxyModel->mapFromSource(index) : index;
}

QString ResourceWidgetPrivate::rootPath() const
{
    return (fsModel ? fsModel->rootPath() : QStringLiteral("/"));
}

class DirFilterProxyModel : public QSortFilterProxyModel
{
public:
    enum Roles
    {
        UrlRole = Qt::UserRole + 1,
        SystemDirRole = Qt::UserRole + 2,
        UserDirRole = Qt::UserRole + 3,
        EnabledRole = Qt::UserRole + 4
    };

    DirFilterProxyModel(QObject *parent = 0);

    void setSourceModel(QAbstractItemModel *sourceModel) override;
    inline int columnCount(const QModelIndex &parent = QModelIndex()) const override { Q_UNUSED(parent) return 1; }
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    QFileSystemModel *fileSystemModel = nullptr;
};

class DirModel : public QStandardItemModel
{
public:
    DirModel(QObject *parent = nullptr) : QStandardItemModel(parent) { }

};

class DirTreeView : public QTreeView
{
    Q_OBJECT
public:
    DirTreeView(QWidget *parent = nullptr);

    void setResourcePrivate(ResourceWidgetPrivate *d_pointer);
    void setModel(QAbstractItemModel *model) override;
    void setRootIndex(const QModelIndex &index) override;
    QModelIndex mapFromSource(const QModelIndex &source) const;
    QModelIndex mapToSource(const QModelIndex &index) const;
    void select(const QModelIndex &index);

public slots:
    void loadUserDirectory(const QString &directory);

private slots:
    void removeEntry();
#if QT_CONFIG(menu)
    void showContextMenu(const QPoint &position);
#endif

private:
    ResourceWidgetPrivate *d_ptr;
    QSortFilterProxyModel *proxyModel = nullptr;
};

class ResourceListView : public QListView
{
    Q_OBJECT
public:
    ResourceListView(QWidget *parent = nullptr) : QListView(parent) {}
    void setResourcePrivate(ResourceWidgetPrivate *d_pointer);
    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    ResourceWidgetPrivate *d_ptr;
};

class ResourceTreeView : public QTreeView
{
    Q_OBJECT
public:
    ResourceTreeView(QWidget *parent = nullptr) : QTreeView(parent) {}
    void setResourcePrivate(ResourceWidgetPrivate *d_pointer);
    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    ResourceWidgetPrivate *d_ptr;
};

class DirDelegate : public QStyledItemDelegate
{
 public:
     DirDelegate(QWidget *parent = nullptr) : QStyledItemDelegate(parent) {}

     void initStyleOption(QStyleOptionViewItem *option,
                          const QModelIndex &index) const override
     {
         QStyledItemDelegate::initStyleOption(option,index);
         QVariant value = index.data(DirFilterProxyModel::EnabledRole);
         if (value.isValid())
         {
             //If the bookmark/entry is not enabled then we paint it in gray
             if (!qvariant_cast<bool>(value))
                 option->state &= ~QStyle::State_Enabled;
         }
     }
};

#endif // RESOURCEWIDGET_P_H
