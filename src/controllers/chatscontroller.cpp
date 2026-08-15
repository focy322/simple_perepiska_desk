#include "controllers/chatscontroller.h"

ChatsController::ChatsController(QObject *parent)
    : BaseController{parent}
    , chatService(new ChatService(this))
{
    connect(chatService, &ChatService::getMyChatsInProgress, this, &ChatsController::getMyChatsInProgress);
    connect(chatService, &ChatService::getMyChatsFinished, this, &ChatsController::on_GetMyChatsFinished);
    connect(chatService, &ChatService::getChatMessagesInProgress, this, &ChatsController::getChatMessagesInProgress);
    connect(chatService, &ChatService::getChatMessagesFinished, this, &ChatsController::on_GetChatMessagesFinished);
    connect(chatService, &ChatService::createDirectChatInProgress, this, &ChatsController::createDirectChatInProgress);
    connect(chatService, &ChatService::createDirectChatFinished, this, &ChatsController::on_CreateDirectChatFinished);
    connect(chatService, &ChatService::editMessageFinished, this, &ChatsController::on_EditMessageFinished);
    connect(chatService, &ChatService::deleteMessageFinished, this, &ChatsController::deleteMessageFinished);

}

void ChatsController::requestMyChats(const QString &accToken, RetryableRequest req)
{
    chatService->getMyChats(accToken, req);
}

void ChatsController::requestChatMessages(const unsigned long long &chatId, const QString &accToken, RetryableRequest req)
{
    chatService->getChatMessages(chatId, accToken, req);
}

void ChatsController::requestCreateDirectChat(const unsigned long long &userId, const QString &accToken)
{
    chatService->createDirectChat(userId, accToken);
}

void ChatsController::requestMarkMessageRead(const std::pair<quint64, quint64> &msg, const QString &accToken)
{
    chatService->markMessageRead(msg, accToken);
}

void ChatsController::requestEditMessage(const quint64 messageId, const quint64 chatId, const QString &newText, const QString &accToken)
{
    chatService->editMessage(messageId, chatId, newText, accToken);
}

void ChatsController::requestDeleteMessage(const std::vector<quint64>& messageIds, const quint64 chatId, const bool deleteForAll, const QString &accToken)
{
    chatService->deleteMessage(messageIds, chatId, deleteForAll, accToken);
}

void ChatsController::on_GetMyChatsFinished(const NetworkResult& res,
                                            RetryableRequest reReq, const std::vector<ParsedChatsListArrayObject>& paObjects)
{
    if (!res.ok)
    {
        static int errorCount = 0;
        ++errorCount;
        reReq.retryCount = errorCount;
        emit errorOccurred(res, reReq);
    }
    emit getMyChatsFinished(res, paObjects);
}

void ChatsController::on_GetChatMessagesFinished(const NetworkResult& res, RetryableRequest reReq,
                                                 const unsigned long long chatId, const std::vector<ParsedChatMessagesArrayObject>& paObjects)
{
    if (!res.ok)
    {
        static int errorCount = 0;
        ++errorCount;
        reReq.retryCount = errorCount;
        emit errorOccurred(res, reReq);
    }
    emit getChatMessagesFinished(res, chatId, paObjects);
}

void ChatsController::on_CreateDirectChatFinished(const NetworkResult& res)
{
    emit createDirectChatFinished(res);
}

void ChatsController::on_EditMessageFinished(const NetworkResult& res)
{
    emit editMessageFinished(res);
}

void ChatsController::on_DeleteMessageFinished(const NetworkResult& res)
{
    emit deleteMessageFinished(res);
}
