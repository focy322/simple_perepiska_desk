#ifndef CHATSCONTROLLER_H
#define CHATSCONTROLLER_H

#include <QObject>
#include "chatservice.h"


class ChatsController : public QObject
{
    Q_OBJECT
public:
    explicit ChatsController(QObject *parent = nullptr);
    void requestMyChats(const QString &accToken);
    void requestChatMessages(const unsigned long long &chatId, const QString &accToken);
    void requestCreateDirectChat(const unsigned long long &userId, const QString &accToken);
    void requestMarkMessageRead(const std::pair<quint64, quint64> &msg, const QString &accToken);

private:
    ChatService *chatService;   //!< Дергает API-шку

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );
    void getChatMessagesInProgress();
    void getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {});
    void createDirectChatInProgress();
    void createDirectChatFinished(const NetworkResult &res);
};

#endif // CHATSCONTROLLER_H
