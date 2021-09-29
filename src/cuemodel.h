/*
 * Copyright (c) 2019-2021, Ilya Kotov <iokotov@astralinux.ru>
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

#ifndef CUEMODEL_H
#define CUEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QMap>

class CueModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CueModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool isEmpty() const;
    int count() const;
    int offset(int track) const;
    void setOffset(int track, int offset);
    void addTrack(const QString &performer, const QString &title, int offset);

    enum MetaDataKey
    {
        PERFORMER = 0,
        TITLE,
        GENRE,
        DATE,
        COMMENT,
        FILE
    };
    void setMetaData(MetaDataKey key, const QString &value);
    void clear();
    void removeTrack(int idx);
    QByteArray generate();

signals:
    void countChanged();

private:

    struct CueItem
    {
        QString performer;
        QString title;
        int offset;
    };
    QList<CueItem> m_items;
    QMap<MetaDataKey, QString> m_metaData;
};

#endif // CUEMODEL_H
