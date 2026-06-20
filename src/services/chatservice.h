#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include "errortypes.h"
#include "endpoints.h"

struct ParsedChatsListArrayObject
{
    QString chatName;
    QString type;
    QString username;
    QString nickname;
    QString userAvatarFileUrl;
    QString lastMessage;
    QString lastMessageTimestamp;
    QString lastMessageAttachmentType;
    unsigned long long chatId = ULONG_LONG_MAX;
    unsigned long long chatAvatarFileId = ULONG_LONG_MAX;
    unsigned long long userId = ULONG_LONG_MAX;
    unsigned long long lastMessageId = ULONG_LONG_MAX;
    unsigned long long lastMessageSenderId = ULONG_LONG_MAX;
    unsigned long long lastMessageAttachmentsCount = 0;
    unsigned int unreadCount = 0;
    bool lastMessageHasAttachments = false;
};

struct ParsedChatMessagesArrayObject
{
    QString message;
    QString timestamp;
    QString clientMessageId;    //<! Uuid
    QString editedAt;
    QString Uuid;
    QString readAt;
    QJsonArray attachments;
    unsigned long long messageId = ULONG_LONG_MAX;
    unsigned long long senderId = ULONG_LONG_MAX;
    unsigned long long chatId = ULONG_LONG_MAX;
    unsigned int attachmentsCount = 0;
    bool isPending = false;
    bool read = false;
    bool edited = false;
    bool hasAttachments = false;
};


class ChatService : public QObject
{
    Q_OBJECT
public:
    explicit ChatService(QObject *parent = nullptr);

    void getMyChats(const QString &accToken);

    void getChatMessages(const unsigned long long &chatId, const QString &accToken);

    const std::vector<ParsedChatsListArrayObject> parseChatsListArray(const QJsonDocument &doc);

    const std::vector<ParsedChatMessagesArrayObject> parseChatMessagesArray(const QJsonDocument &doc);

    void createDirectChat(const unsigned long long &userId, const QString &accToken);

    void markMessageRead(const std::pair<quint64, quint64> &msg, const QString &accToken);



private:
    QNetworkAccessManager *network;                     //!< Указатель на объект для работы с запросами
    //TODO : А зачем когда у меня есть baseHttpUrl
    QString baseUrl;                                    //!< Базовый адрес API
    QString myChatsUrl;                                 //!< Адрес для списка чатов
    QString chatMessagesUrl;                            //!< Адрес для списка сообщений конкретного чата
    QString createDirectChatUrl;                        //!< Адрес для создания private-чата
    QString markMessageReadUrl;

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );
    void getChatMessagesInProgress();
    void getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {} );
    void createDirectChatInProgress();
    void createDirectChatFinished(const NetworkResult &res);
};


#endif // CHATSERVICE_H
