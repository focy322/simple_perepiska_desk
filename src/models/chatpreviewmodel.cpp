#include "chatpreviewmodel.h"
#include "chatlistmodel.h"

ChatPreviewModel::ChatPreviewModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_message.message = tr("Так выглядит сообщение");
    m_message.timestamp = QStringLiteral("2000-01-01T12:30:00");
}

int ChatPreviewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant ChatPreviewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.row() != 0 || index.column() != 0)
        return {};

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case ChatListModel::ChatNameRole:
        return tr("Пример чата");
    case ChatListModel::ChatIdRole:
        return QVariant::fromValue<qulonglong>(0);
    case ChatListModel::LastMessageRole:
        return QVariant::fromValue(m_message);
    case ChatListModel::UnreadCountRole:
        return 2u;
    default:
        return {};
    }
}
