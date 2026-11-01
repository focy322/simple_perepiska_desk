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
    QString leftTimestamp = left.data(ChatListModel::LastMessageRole).value<MessagePreview>().timestamp;
    QString rightTimestamp = right.data(ChatListModel::LastMessageRole).value<MessagePreview>().timestamp;

    QDateTime dateLeft = QDateTime::fromString(leftTimestamp,Qt::ISODateWithMs);
    if ( !dateLeft.isValid() )
        dateLeft = QDateTime::fromString(leftTimestamp,Qt::ISODate);

    QDateTime dateRight = QDateTime::fromString(rightTimestamp,Qt::ISODateWithMs);
    if ( !dateRight.isValid() )
        dateRight = QDateTime::fromString(rightTimestamp,Qt::ISODate);

    return dateLeft < dateRight;
}
