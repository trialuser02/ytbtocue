#include <QtDebug>
#include <QTimeEdit>
#include "tracklistitemdelegate.h"

TrackListItemDelegate::TrackListItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{}

QWidget *TrackListItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
    QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
    if(timeEdit)
        timeEdit->setDisplayFormat("h:mm:ss");

    return editor;
}
