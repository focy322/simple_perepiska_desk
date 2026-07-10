#ifndef CHATLISTMODEL_H
#define CHATLISTMODEL_H

#include <QAbstractListModel>
#include <vector>

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
        LastMessageAttachmentsCountRole
    };

    explicit ChatListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChats(const std::vector<ParsedChatsListArrayObject> &chats);
    void clear();

private:
    std::vector<ParsedChatsListArrayObject> m_chats;
};

#endif // CHATLISTMODEL_H
