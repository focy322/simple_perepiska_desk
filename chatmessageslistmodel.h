#ifndef CHATMESSAGESLISTMODEL_H
#define CHATMESSAGESLISTMODEL_H

#include <QAbstractListModel>
#include <vector>

#include "chatservice.h"

class ChatMessagesListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum MessageRoles
    {
        MessageIdRole = Qt::UserRole + 1,
        SenderIdRole,
        ChatIdRole,
        MessageTextRole,
        TimestampRole,
        IsPendingRole,
        ReadRole,
        ReadAtRole,
        EditedRole,
        EditedAtRole
    };

    explicit ChatMessagesListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(const std::vector<ParsedChatMessagesArrayObject> &messages);
    void appendMessage(const ParsedChatMessagesArrayObject &message);
    void clear();

private:
    std::vector<ParsedChatMessagesArrayObject> m_messages;
};

#endif // CHATMESSAGESLISTMODEL_H
