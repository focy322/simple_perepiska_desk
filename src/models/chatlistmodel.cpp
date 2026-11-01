#include "models/chatlistmodel.h"

#include <QtNetwork/QNetworkRequest>
#include <QMetaEnum>
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

    QString displayName = chat.chatName;

    switch (role)
    {
    case Qt::DisplayRole:
        return displayName;
    case ChatIdRole:
        return QVariant::fromValue<qulonglong>(chat.chatId);
    case ChatNameRole:
        return displayName;
    case LastMessageRole:
        return QVariant::fromValue(chat.lastMessage);
    case AvatarFileIdRole:
        return QVariant::fromValue<qulonglong>(chat.chatAvatarFileId);
    case ChatTypeRole:
        return chat.type;
    case InterlocutorRole:
        return QVariant::fromValue(chat.interlocutor);
    case UnreadCountRole:
        return chat.unreadCount;
    case AvatarPixmapRole:
    {
        if (!chat.interlocutor.userAvatarFileUrl.isEmpty() && m_avatarCache.contains(chat.interlocutor.userAvatarFileUrl)) {
            return m_avatarCache[chat.interlocutor.userAvatarFileUrl];
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
    const auto meta = QMetaEnum::fromType<ChatRoles>();

    for (int i = 0; i < meta.keyCount(); ++i) {
        QByteArray name = meta.key(i);
        if (name.endsWith("Role"))
            name.chop(4);

        roles.insert(meta.value(i), name);
    }

    return roles;
}

void ChatListModel::setChats(const std::vector<ParsedChatsListArrayObject> &chats)
{
    beginResetModel();
    m_chats = chats;
    endResetModel();
    fetchAvatars();
}

void ChatListModel::updateChat(const ParsedChatsListArrayObject &chat)
{
    beginResetModel();
    auto it = std::ranges::find_if(m_chats, [ &chat ](const ParsedChatsListArrayObject & other){
        return chat.chatId == other.chatId;
    });
    if (it != m_chats.end())
        *it = chat;

    endResetModel();
}

void ChatListModel::clear()
{
    beginResetModel();
    m_chats.clear();
    endResetModel();
}

void ChatListModel::decreaseUnreadCount(quint64 chatId, int count)
{
    for (size_t i = 0; i < m_chats.size(); ++i) {
        if (m_chats[i].chatId == chatId) {
            if (m_chats[i].unreadCount >= static_cast<unsigned int>(count)) {
                m_chats[i].unreadCount -= count;
            } else {
                m_chats[i].unreadCount = 0;
            }
            QModelIndex idx = index(static_cast<int>(i), 0);
            emit dataChanged(idx, idx, {UnreadCountRole});
            break;
        }
    }
}

void ChatListModel::setUnreadCount(quint64 chatId, int count)
{
    for (size_t i = 0; i < m_chats.size(); ++i) {
        if (m_chats[i].chatId == chatId) {
            m_chats[i].unreadCount = count;
            QModelIndex idx = index(static_cast<int>(i), 0);
            emit dataChanged(idx, idx, {UnreadCountRole});
            break;
        }
    }
}

void ChatListModel::fetchAvatars()
{
    for (size_t i = 0; i < m_chats.size(); ++i) {
        QString url = m_chats[i].interlocutor.userAvatarFileUrl;
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
