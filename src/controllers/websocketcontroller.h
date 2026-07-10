#ifndef WEBSOCKETCONTROLLER_H
#define WEBSOCKETCONTROLLER_H

#include <QObject>
#include "services/websocketservice.h"

class WebsocketController : public QObject
{
    Q_OBJECT
public:
    explicit WebsocketController(QObject *parent = nullptr);
    void requestConnectSocket(const QString &accessToken);
    void requestDisconnectSocket();
    void requestSendMessage(const ParsedChatMessagesArrayObject &message);

private:
    WebsocketService *websocketService; //!< Дергает сокет

signals:
    void socketConnectionInProgress();
    void socketConnectionFinished(const NetworkResult &res);
    void socketDisonnectionInProgress();
    void socketDisonnectionFinished(const NetworkResult &res);
    void sendingMessageInProgress();
    void sendingMessageFinished(const NetworkResult &res);
    void newMessageRecieved(const ParsedChatMessagesArrayObject &newMessage);
    void messageAccepted(const ParsedMessageAcceptedObject &msgAccObj);
    void messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId);
};

#endif // WEBSOCKETCONTROLLER_H
