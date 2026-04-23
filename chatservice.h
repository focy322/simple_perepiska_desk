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

// TODO: обновить
struct ParsedChatsListArrayObject
{
    unsigned long long chatId = 0;
    QString chatName;
    unsigned long long chatAvatarFileId = 0;
    QString type;

    unsigned long long userId = 0;
    QString username;
    unsigned long long userAvatarFileId = 0;

    unsigned long long lastMessageId = 0;
    unsigned long long lastMessageSenderId = 0;
    unsigned long long lastMessageChatId = 0;
    QString lastMessage;
    unsigned long long lastMessageFileId = 0;
    QString lastMessageTimestamp;
    bool lastMessageRead = false;
    QString lastMessageReadAt;
    bool lastMessageEdited = false;
    QString lastMessageEditedAt;
};

// TODO: обновить
struct ParsedChatMessagesArrayObject
{
    unsigned long long messageId = 0;
    unsigned long long senderId = 0;
    unsigned long long chatId = 0;
    QString message;
    QString timestamp;
    QString clientMessageId;    //<! Uuid
    bool isPending = false;
    bool read = false;
    QString readAt;
    bool edited = false;
    QString editedAt;
    QString Uuid;
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


private:
    QNetworkAccessManager *network;                     //!< Указатель на объект для работы с запросами
    QString baseUrl;                                    //!< Базовый адрес API
    QString myChatsUrl;                                 //!< Адрес для списка чатов
    QString chatMessagesUrl;                            //!< Адрес для списка сообщений конкретного чата
    QString createDirectChatUrl;                        //!< Адрес для создания private-чата

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );
    void getChatMessagesInProgress();
    void getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {} );
    void createDirectChatInProgress();
    void createDirectChatFinished(const NetworkResult &res);
};


#endif // CHATSERVICE_H
