#include "chatlistmodel.h"

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ChatListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_chats.size());
}

QVariant ChatListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_chats.size()))
        return {};

    const ParsedChatsListArrayObject &chat = m_chats[static_cast<size_t>(row)];

    QString displayName = chat.chatName.trimmed();
    if (displayName.isEmpty())
    {
        if (chat.type.compare("private", Qt::CaseInsensitive) == 0 && !chat.username.trimmed().isEmpty())
            displayName = chat.username.trimmed();
        else
            displayName = QString("Chat %1").arg(chat.chatId);
    }

    switch (role)
    {
    case Qt::DisplayRole:
        return displayName;
    case ChatIdRole:
        return QVariant::fromValue<qulonglong>(chat.chatId);
    case ChatNameRole:
        return displayName;
    case LastMessageRole:
        return chat.lastMessage.trimmed();
    case LastMessageTimestampRole:
        return chat.lastMessageTimestamp;
    case AvatarFileIdRole:
        return QVariant::fromValue<qulonglong>(chat.chatAvatarFileId);
    case ChatTypeRole:
        return chat.type;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ChatIdRole] = "chatId";
    roles[ChatNameRole] = "chatName";
    roles[LastMessageRole] = "lastMessage";
    roles[LastMessageTimestampRole] = "lastMessageTimestamp";
    roles[AvatarFileIdRole] = "avatarFileId";
    roles[ChatTypeRole] = "chatType";
    return roles;
}

void ChatListModel::setChats(const std::vector<ParsedChatsListArrayObject> &chats)
{
    beginResetModel();
    m_chats = chats;
    endResetModel();
}

void ChatListModel::clear()
{
    beginResetModel();
    m_chats.clear();
    endResetModel();
}
