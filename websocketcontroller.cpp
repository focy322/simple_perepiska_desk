#include "websocketcontroller.h"

WebsocketController::WebsocketController(QObject *parent)
    : QObject{parent}
    , websocketService(new WebsocketService(this))
{
    connect(websocketService, &WebsocketService::socketConnectionInProgress, this, &WebsocketController::socketConnectionInProgress);
    connect(websocketService, &WebsocketService::socketConnectionFinished, this, &WebsocketController::socketConnectionFinished);
    connect(websocketService, &WebsocketService::socketDisonnectionInProgress, this, &WebsocketController::socketDisonnectionInProgress);
    connect(websocketService, &WebsocketService::socketDisonnectionFinished, this, &WebsocketController::socketDisonnectionFinished);
    connect(websocketService, &WebsocketService::sendingMessageInProgress, this, &WebsocketController::sendingMessageInProgress);
    connect(websocketService, &WebsocketService::sendingMessageFinished, this, &WebsocketController::sendingMessageFinished);
    connect(websocketService, &WebsocketService::newMessageRecieved, this, &WebsocketController::newMessageRecieved);
    connect(websocketService, &WebsocketService::messageAccepted, this, &WebsocketController::messageAccepted);
    connect(websocketService, &WebsocketService::messageMarkedRead, this, &WebsocketController::messageMarkedRead);
}

void WebsocketController::requestConnectSocket(const QString &accessToken)
{
    websocketService->connectSocket(accessToken);
}


void WebsocketController::requestDisconnectSocket()
{

}

void WebsocketController::requestSendMessage(const ParsedChatMessagesArrayObject &message)
{
    websocketService->sendMessage(message);
}


