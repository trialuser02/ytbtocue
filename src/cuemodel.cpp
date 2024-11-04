/*
 * Copyright (c) 2019-2024, Ilya Kotov <iokotov@astralinux.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QtDebug>
#include <QTime>
#include "utils.h"
#include "cuemodel.h"

CueModel::CueModel(QObject *parent) : QAbstractListModel(parent)
{}

QVariant CueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    if(section == TrackColumn)
        return tr("Track");
    if(section == PerformerColumn)
        return tr("Performer");
    if(section == TitleColumn)
        return tr("Title");
    if(section == OffsetColumn)
        return tr("Offset");

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
    if(!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole) || index.row() >= m_items.count())
        return QVariant();

    int column = index.column();
    int row = index.row();

    if(column == TrackColumn)
        return QString::number(row + 1);
    if(column == PerformerColumn)
        return m_items[row].performer;
    if(column == TitleColumn)
        return m_items[row].title;
    if(column == OffsetColumn && role == Qt::DisplayRole)
        return Utils::formatDuration(m_items[row].offset);
    if(column == OffsetColumn && role == Qt::EditRole)
        return QTime(0, 0).addSecs(m_items[row].offset);

    return QVariant();
}

bool CueModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid() || role != Qt::EditRole)
        return false;

    int row = index.row();
    int column = index.column();

    if(column == PerformerColumn)
    {
        m_items[row].performer = value.toString();
        emit dataChanged(index, index, { role });
        return true;
    }
    if(column == TitleColumn)
    {
        m_items[row].title = value.toString();
        emit dataChanged(index, index, { role });
        return true;
    }
    if(column == OffsetColumn)
    {
        QTime time = value.toTime();
        m_items[row].offset = time.hour() * 3600 + time.minute() * 60 + time.second();
        emit dataChanged(index, index, { role });
        return true;
    }
    return false;
}

Qt::ItemFlags CueModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractListModel::flags(index);
    if(index.column() == PerformerColumn || index.column() == TitleColumn || index.column() == OffsetColumn)
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool CueModel::isEmpty() const
{
    return m_items.isEmpty();
}

int CueModel::count() const
{
    return m_items.count();
}

int CueModel::offset(int track) const
{
    return m_items.at(track).offset;
}

void CueModel::setOffset(int track, int offset)
{
    m_items[track].offset = offset;
}

void CueModel::addTrack(const QString &performer, const QString &title, int offset)
{
    qDebug() << performer << title << offset;
    beginInsertRows(QModelIndex(), m_items.count(),  m_items.count());
    CueItem item { .performer = performer, .title = title, .offset = offset };
    m_items << item;
    endInsertRows();
    emit countChanged();
}

void CueModel::setMetaData(CueModel::MetaDataKey key, const QString &value)
{
    m_metaData[key] = value.trimmed();
}

void CueModel::clear()
{
    beginResetModel();
    m_items.clear();
    m_metaData.clear();
    endResetModel();
    emit countChanged();
}

void CueModel::removeTrack(int idx)
{
    beginRemoveRows(QModelIndex(), idx, idx);
    m_items.removeAt(idx);
    endRemoveRows();
    emit countChanged();
}

QByteArray CueModel::generate()
{
    QString out;
    if(!m_metaData.value(GENRE).isEmpty())
        out += QStringLiteral("REM GENRE \"%1\"\n").arg(m_metaData.value(GENRE));
    if(!m_metaData.value(DATE).isEmpty())
        out += QStringLiteral("REM DATE %1\n").arg(m_metaData.value(DATE));
    if(!m_metaData.value(COMMENT).isEmpty())
        out += QStringLiteral("REM COMMENT \"%1\"\n").arg(m_metaData.value(COMMENT));
    if(!m_metaData.value(PERFORMER).isEmpty())
        out += QStringLiteral("PERFORMER \"%1\"\n").arg(m_metaData.value(PERFORMER));
    if(!m_metaData.value(TITLE).isEmpty())
        out += QStringLiteral("TITLE \"%1\"\n").arg(m_metaData.value(TITLE));
    if(!m_metaData.value(FILE).isEmpty())
        out += QStringLiteral("FILE \"%1\" WAVE\n").arg(m_metaData.value(FILE));

    for(int i = 0; i < m_items.count(); ++i)
    {
        out += QStringLiteral("  TRACK %1 AUDIO\n").arg(i + 1, 2, 10, QLatin1Char('0'));
        out += QStringLiteral("    TITLE \"%1\"\n").arg(m_items[i].title);
        if(!m_items[i].performer.isEmpty())
            out += QStringLiteral("    PERFORMER \"%1\"\n").arg(m_items[i].performer);
        out += QStringLiteral("    INDEX 01 %1:%2:00\n").arg(m_items[i].offset / 60,  2, 10, QLatin1Char('0'))
                .arg(m_items[i].offset % 60,  2, 10, QLatin1Char('0'));
    }
    return out.toUtf8();
}
