#include <QtDebug>
#include "utils.h"
#include "cuemodel.h"

CueModel::CueModel(QObject *parent) : QAbstractListModel(parent)
{

}

QVariant CueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    if(section == 0)
        return tr("Track");
    else if(section == 1)
        return tr("Performer");
    else if(section == 2)
        return tr("Title");
    else if(section == 3)
        return tr("Offset");
    else
        return QString();
}

int CueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_items.count();
}

int CueModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

QVariant CueModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || role != Qt::DisplayRole || index.row() >= m_items.count())
        return QVariant();

    int column = index.column();
    int row = index.row();

    if(column == 0)
        return QString::number(row + 1);
    else if(column == 1)
        return m_items[row].performer;
    else if(column == 2)
        return m_items[row].title;
    else if(column == 3)
        return Utils::formatDuration(m_items[row].offset);

    return QVariant();
}

void CueModel::addTrack(const QString &performer, const QString &title, int offset)
{
    qDebug() << performer << title << offset;
    beginInsertRows(QModelIndex(), m_items.count(),  m_items.count());
    CueItem item { .performer = performer, .title = title, .offset = offset };
    m_items << item;
    endInsertRows();
}

void CueModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}
