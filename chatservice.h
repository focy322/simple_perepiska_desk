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

struct ParsedArrayObject
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

class ChatService : public QObject
{
    Q_OBJECT
public:
    explicit ChatService(QObject *parent = nullptr);

    void getMyChats(const QString &accToken);

    const std::vector<ParsedArrayObject> parseArray(const QJsonDocument &doc);

private:
    QNetworkAccessManager *network;                     //!< Указатель на объект для работы с запросами
    QString baseUrl;                                    //!< Базовый адрес API
    QString myChatsUrl;                                 //!< Адрес для получения чатов

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const AuthResult &res, const std::vector<ParsedArrayObject>& paObjects = {} );
};


#endif // CHATSERVICE_H
