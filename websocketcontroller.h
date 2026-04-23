#ifndef WEBSOCKETCONTROLLER_H
#define WEBSOCKETCONTROLLER_H

#include <QObject>
#include "websocketservice.h"

class WebsocketController : public QObject
{
    Q_OBJECT
public:
    explicit WebsocketController(QObject *parent = nullptr);
    void requestConnectSocket(const QString &accessToken);
    void requestDisconnectSocket();
    void requestSendMessage(const unsigned long long &chatId, const QString &message, const QString &Uuid, const unsigned long long &fileId = 0);

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
};

#endif // WEBSOCKETCONTROLLER_H
