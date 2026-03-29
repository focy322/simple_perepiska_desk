#include "chatmessageslistmodel.h"

ChatMessagesListModel::ChatMessagesListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ChatMessagesListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_messages.size());
}

QVariant ChatMessagesListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_messages.size()))
        return {};

    const ParsedChatMessagesArrayObject &message = m_messages[static_cast<size_t>(row)];

    switch (role)
    {
    case Qt::DisplayRole:
        return message.message;
    case MessageIdRole:
        return QVariant::fromValue<qulonglong>(message.messageId);
    case SenderIdRole:
        return QVariant::fromValue<qulonglong>(message.senderId);
    case ChatIdRole:
        return QVariant::fromValue<qulonglong>(message.chatId);
    case MessageTextRole:
        return message.message;
    case FileIdRole:
        return QVariant::fromValue<qulonglong>(message.fileId);
    case TimestampRole:
        return message.timestamp;
    case ReadRole:
        return message.read;
    case ReadAtRole:
        return message.readAt;
    case EditedRole:
        return message.edited;
    case EditedAtRole:
        return message.editedAt;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessagesListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MessageIdRole] = "messageId";
    roles[SenderIdRole] = "senderId";
    roles[ChatIdRole] = "chatId";
    roles[MessageTextRole] = "messageText";
    roles[FileIdRole] = "fileId";
    roles[TimestampRole] = "timestamp";
    roles[ReadRole] = "read";
    roles[ReadAtRole] = "readAt";
    roles[EditedRole] = "edited";
    roles[EditedAtRole] = "editedAt";
    return roles;
}

void ChatMessagesListModel::setMessages(const std::vector<ParsedChatMessagesArrayObject> &messages)
{
    beginResetModel();
    m_messages = messages;
    endResetModel();
}

void ChatMessagesListModel::appendMessage(const ParsedChatMessagesArrayObject &message)
{
    const int insertRow = static_cast<int>(m_messages.size());
    beginInsertRows(QModelIndex(), insertRow, insertRow);
    m_messages.push_back(message);
    endInsertRows();
}

void ChatMessagesListModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}
