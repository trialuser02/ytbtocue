#ifndef CUEMODEL_H
#define CUEMODEL_H

#include <QAbstractListModel>
#include <QList>


class CueModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CueModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void addTrack(const QString &performer, const QString &title, int offset);
    void setFile(const QString &file);
    const QString &file() const;
    void setAlbum(const QString &album);
    const QString &album() const;
    void clear();
    QByteArray generate();

private:

    struct CueItem
    {
        QString performer;
        QString title;
        int offset = 0;
    };
    QList<CueItem> m_items;
    QString m_file, m_album;
};

#endif // CUEMODEL_H
