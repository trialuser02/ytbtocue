#ifndef TRACKLISTITEMDELEGATE_H
#define TRACKLISTITEMDELEGATE_H

#include <QObject>
#include <QWidget>
#include <QStyledItemDelegate>

class TrackListItemDelegate : public QStyledItemDelegate
{
public:
    TrackListItemDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

};

#endif // TRACKLISTITEMDELEGATE_H
