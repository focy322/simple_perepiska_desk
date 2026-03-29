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

struct ParsedChatMessagesArrayObject
{
    unsigned long long messageId = 0;
    unsigned long long senderId = 0;
    unsigned long long chatId = 0;
    QString message;
    unsigned long long fileId = 0;
    QString timestamp;
    bool read = false;
    QString readAt;
    bool edited = false;
    QString editedAt;
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


private:
    QNetworkAccessManager *network;                     //!< Указатель на объект для работы с запросами
    QString baseUrl;                                    //!< Базовый адрес API
    QString myChatsUrl;                                 //!< Адрес для списка чатов
    QString chatMessagesUrl;                            //!< Адрес для списка сообщений конкретного чата

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const AuthResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );
    void getChatMessagesInProgress();
    void getChatMessagesFinished(const AuthResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {} );
};


#endif // CHATSERVICE_H
