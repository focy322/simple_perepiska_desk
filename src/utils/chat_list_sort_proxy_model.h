//
// Created by belya on 06.09.2026.
//

#ifndef CHATLISTSORTPROXYMODEL_H
#define CHATLISTSORTPROXYMODEL_H

#include <QSortFilterProxyModel>

class ChatListSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ChatListSortProxyModel(QObject *parent = nullptr);
protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


#endif //CHATLISTSORTPROXYMODEL_H
