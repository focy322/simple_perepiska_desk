#ifndef CHATLISTMODEL_H
#define CHATLISTMODEL_H

#include <QAbstractListModel>
#include <vector>
#include <QPixmap>
#include <QMap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "services/chatservice.h"

class ChatListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ChatRoles
    {
        ChatIdRole = Qt::UserRole + 1,
        ChatNameRole,
        LastMessageRole,
        LastMessageTimestampRole,
        AvatarFileIdRole,
        ChatTypeRole,
        UserIdRole,
        UsernameRole,
        NicknameRole,
        UserAvatarFileUrlRole,
        UnreadCountRole,
        LastMessageHasAttachmentsRole,
        LastMessageAttachmentTypeRole,
        LastMessageAttachmentsCountRole,
        AvatarPixmapRole
    };

    explicit ChatListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChats(const std::vector<ParsedChatsListArrayObject> &chats);
    void clear();
    void decreaseUnreadCount(quint64 chatId, int count);
    void setUnreadCount(quint64 chatId, int count);

private slots:
    void onAvatarDownloaded(QNetworkReply *reply, int row, const QString &url);

private:
    std::vector<ParsedChatsListArrayObject> m_chats;
    QNetworkAccessManager *m_networkManager;
    QMap<QString, QPixmap> m_avatarCache;
    void fetchAvatars();
};

#endif // CHATLISTMODEL_H
