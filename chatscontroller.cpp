#include "chatscontroller.h"

ChatsController::ChatsController(QObject *parent)
    : QObject{parent}
    , chatService(new ChatService(this))
{
    connect(chatService, &ChatService::getMyChatsInProgress, this, &ChatsController::getMyChatsInProgress);
    connect(chatService, &ChatService::getMyChatsFinished, this, &ChatsController::getMyChatsFinished);
}

void ChatsController::requestMyChats(const QString &accToken)
{
    chatService->getMyChats(accToken);
}
