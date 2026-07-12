#include "models/chatlistmodel.h"
#include <QtNetwork/QNetworkRequest>
#include <QUrl>

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

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
        if (chat.type.compare("private", Qt::CaseInsensitive) == 0)
        {
            const QString nickname = chat.nickname.trimmed();
            const QString username = chat.username.trimmed();
            if (!nickname.isEmpty())
                displayName = nickname;
            else if (!username.isEmpty())
                displayName = username;
        }
        if (displayName.isEmpty())
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
    case UserIdRole:
        return chat.userId;
    case UsernameRole:
        return chat.username;
    case NicknameRole:
        return chat.nickname;
    case UserAvatarFileUrlRole:
        return chat.userAvatarFileUrl;
    case UnreadCountRole:
        return static_cast<uint>(chat.unreadCount);
    case LastMessageHasAttachmentsRole:
        return chat.lastMessageHasAttachments;
    case LastMessageAttachmentTypeRole:
        return chat.lastMessageAttachmentType;
    case LastMessageAttachmentsCountRole:
        return QVariant::fromValue<qulonglong>(chat.lastMessageAttachmentsCount);
    case AvatarPixmapRole:
    {
        if (!chat.userAvatarFileUrl.isEmpty() && m_avatarCache.contains(chat.userAvatarFileUrl)) {
            return m_avatarCache[chat.userAvatarFileUrl];
        }
        return QVariant();
    }
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
    roles[UserIdRole] = "userId";
    roles[UsernameRole] = "username";
    roles[NicknameRole] = "nickname";
    roles[UserAvatarFileUrlRole] = "userAvatarFileUrl";
    roles[UnreadCountRole] = "unreadCount";
    roles[LastMessageHasAttachmentsRole] = "lastMessageHasAttachments";
    roles[LastMessageAttachmentTypeRole] = "lastMessageAttachmentType";
    roles[LastMessageAttachmentsCountRole] = "lastMessageAttachmentsCount";
    return roles;
}

void ChatListModel::setChats(const std::vector<ParsedChatsListArrayObject> &chats)
{
    beginResetModel();
    m_chats = chats;
    endResetModel();
    fetchAvatars();
}

void ChatListModel::clear()
{
    beginResetModel();
    m_chats.clear();
    endResetModel();
}

void ChatListModel::fetchAvatars()
{
    for (size_t i = 0; i < m_chats.size(); ++i) {
        QString url = m_chats[i].userAvatarFileUrl;
        if (!url.isEmpty() && !m_avatarCache.contains(url)) {
            QNetworkRequest request((QUrl(url)));
            QNetworkReply *reply = m_networkManager->get(request);
            connect(reply, &QNetworkReply::finished, this, [this, reply, i, url]() {
                onAvatarDownloaded(reply, static_cast<int>(i), url);
            });
        }
    }
}

void ChatListModel::onAvatarDownloaded(QNetworkReply *reply, int row, const QString &url)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            m_avatarCache[url] = pixmap;
            QModelIndex idx = index(row, 0);
            emit dataChanged(idx, idx, {AvatarPixmapRole});
        }
    }
}
