#include "editordelegate.h"

#include <QAbstractItemView>
#include <QPainter>
#include <QSpinBox>
#include <QtDebug>

void EditorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if(index.column() == 1)
    {
        QPaintDevice* originalDevice = painter->device();
//        if (option.state & QStyle::State_HasFocus)
//            painter->fillRect(option.rect, option.palette.highlight());

        QVariant indexValue = index.data(Qt::UserRole);
        if(indexValue.canConvert<QWidget *>())
        {
            QWidget *w = indexValue.value<QWidget *>();
            if(w)
            {
                setEditorData(w, index);
                w->setGeometry(option.rect.adjusted(80, 0, -80, 0));
                QPoint mappedorigin = painter->deviceTransform().map(QPoint(option.rect.x() + 80, option.rect.y()));
                painter->end();

                w->render(painter->device(), mappedorigin,
                           QRegion(0, 0, option.rect.width(), option.rect.width()),
                           QWidget::RenderFlag::DrawChildren);

                painter->begin(originalDevice);
            }
        }
    }

}

QSize EditorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column() == 1)
    {
        QVariant indexValue = index.data(Qt::UserRole);
        if(indexValue.canConvert<QWidget *>())
        {
            QWidget *w = indexValue.value<QWidget *>();
            if(w)
                return w->sizeHint();
        }
    }
    return QStyledItemDelegate::sizeHint(option, index) + QSize(0, 4);
}

QWidget *EditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return nullptr;
}

void EditorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QStyledItemDelegate::setEditorData(editor, index);
}

void EditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QStyledItemDelegate::setModelData(editor, model, index);
}

void EditorDelegate::commitEditor()
{
    QWidget *editor = qobject_cast<QWidget *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}
