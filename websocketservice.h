#ifndef WEBSOCKETSERVICE_H
#define WEBSOCKETSERVICE_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QWebSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "errortypes.h"
#include "chatservice.h"
#include "endpoints.h"

struct ParsedMessageAcceptedObject
{
    QString clientMessageId;
    QString timestamp;
    unsigned long long messageId = 0;
    unsigned long long chatId = 0;
    bool deduped = false;
};

class WebsocketService : public QObject
{
    Q_OBJECT
public:
    using Handler = void(WebsocketService::*)(const QJsonObject& payload);
    explicit WebsocketService(QObject *parent = nullptr);
    void connectSocket(const QString &accessToken);
    void disconnectSocket();
    void sendMessage(const unsigned long long &chatId, const QString &message, const QString &Uuid, const unsigned long long &fileId);
    void fillHandlersMap();
private:
    QWebSocket *websocket;
    QString baseUrl;
    QString webSocketUrl;
    QTimer *ackFlushTimer;
    QTimer *outgoingMessagesFlushTimer;
    QSet<unsigned long long> pendingDeliveryIds;
    QHash<QString, QJsonObject> pendingOutgoingMessages;
    int ackFlushIntervalMs;
    int outgoingMessagesFlushIntervalMs;
    QHash<QString, Handler> handlersMapByTypeOfMessage; //!< хэш таблица для обработчиков по типу получаемого сообщения

    //!< Обработчики
    void on_pong(const QJsonObject& payload);
    void on_newMessage(const QJsonObject& payload);
    void on_messageAccepted(const QJsonObject& payload);
    void on_ackResult(const QJsonObject& payload);
    void on_readMarked(const QJsonObject& payload);
    void on_userStatus(const QJsonObject& payload);
    void on_error(const QJsonObject& payload);
    //!< Обработчики

    void flushPendingAcks();
    void flushPendingOutgoingMessages();
    void callHandler(const QString &type, const QJsonObject& payload);

signals:
    void socketConnectionInProgress();
    void socketConnectionFinished(const NetworkResult &res);
    void socketDisonnectionInProgress();
    void socketDisonnectionFinished(const NetworkResult &res);
    void sendingMessageInProgress();
    void sendingMessageFinished(const NetworkResult &res);
    void newMessageRecieved(const ParsedChatMessagesArrayObject &newMessage);
    void messageAccepted(const ParsedMessageAcceptedObject &msgAccObj);
private slots:
    void on_textMessageReceived(const QString &message);

};

#endif // WEBSOCKETSERVICE_H
