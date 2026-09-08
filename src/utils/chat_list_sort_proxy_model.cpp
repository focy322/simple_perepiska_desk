//
// Created by belya on 06.09.2026.
//


#include "chat_list_sort_proxy_model.h"
#include "models/chatlistmodel.h"


ChatListSortProxyModel::ChatListSortProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{

}

bool ChatListSortProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QDateTime dateLeft = left.data(ChatListModel::LastMessageTimestampRole).toDateTime();
    QDateTime dateRight = right.data(ChatListModel::LastMessageTimestampRole).toDateTime();

    return dateLeft < dateRight;
}
