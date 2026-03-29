#include "chatscontroller.h"

ChatsController::ChatsController(QObject *parent)
    : QObject{parent}
    , chatService(new ChatService(this))
{
    connect(chatService, &ChatService::getMyChatsInProgress, this, &ChatsController::getMyChatsInProgress);
    connect(chatService, &ChatService::getMyChatsFinished, this, &ChatsController::getMyChatsFinished);
    connect(chatService, &ChatService::getChatMessagesInProgress, this, &ChatsController::getChatMessagesInProgress);
    connect(chatService, &ChatService::getChatMessagesFinished, this, &ChatsController::getChatMessagesFinished);
}

void ChatsController::requestMyChats(const QString &accToken)
{
    chatService->getMyChats(accToken);
}

void ChatsController::requestChatMessages(const unsigned long long &chatId, const QString &accToken)
{
    chatService->getChatMessages(chatId, accToken);
}
