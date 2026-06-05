#include "chatscontroller.h"

ChatsController::ChatsController(QObject *parent)
    : QObject{parent}
    , chatService(new ChatService(this))
{
    connect(chatService, &ChatService::getMyChatsInProgress, this, &ChatsController::getMyChatsInProgress);
    connect(chatService, &ChatService::getMyChatsFinished, this, &ChatsController::getMyChatsFinished);
    connect(chatService, &ChatService::getChatMessagesInProgress, this, &ChatsController::getChatMessagesInProgress);
    connect(chatService, &ChatService::getChatMessagesFinished, this, &ChatsController::getChatMessagesFinished);
    connect(chatService, &ChatService::createDirectChatInProgress, this, &ChatsController::createDirectChatInProgress);
    connect(chatService, &ChatService::createDirectChatFinished, this, &ChatsController::createDirectChatFinished);

}

void ChatsController::requestMyChats(const QString &accToken)
{
    chatService->getMyChats(accToken);
}

void ChatsController::requestChatMessages(const unsigned long long &chatId, const QString &accToken)
{
    chatService->getChatMessages(chatId, accToken);
}

void ChatsController::requestCreateDirectChat(const unsigned long long &userId, const QString &accToken)
{
    chatService->createDirectChat(userId, accToken);
}

void ChatsController::requestMarkMessageRead(const std::pair<quint64, quint64> &msg, const QString &accToken)
{
    chatService->markMessageRead(msg, accToken);
}

