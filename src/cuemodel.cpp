#include <QtDebug>
#include "utils.h"
#include "cuemodel.h"

CueModel::CueModel(QObject *parent) : QAbstractListModel(parent)
{}

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

void CueModel::setFile(const QString &file)
{
    m_file = file;
}

const QString &CueModel::file() const
{
    return m_file;
}

void CueModel::clear()
{
    beginResetModel();
    m_items.clear();
    m_file.clear();
    endResetModel();
}

QByteArray CueModel::generate()
{
    QString out;
    out += QString("FILE \"%1\"\n").arg(m_file);
    for(int i = 0; i < m_items.count(); ++i)
    {
        out += QString("  TRACK %1 AUDIO\n").arg(i + 1, 2, 10, QChar('0'));
        out += QString("    TITLE \"%1\"\n").arg(m_items[i].title);
        if(!m_items[i].performer.isEmpty())
            out += QString("    PERFORMER \"%1\"\n").arg(m_items[i].performer);
        out += QString("    INDEX 01 %1:%2\n").arg(m_items[i].offset / 60,  2, 10, QChar('0'))
                .arg(m_items[i].offset % 60,  2, 10, QChar('0'));
    }
    return out.toUtf8();
}
