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

private:
    ChatService *chatService;   //!< Дергает API-шку

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const AuthResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );
    void getChatMessagesInProgress();
    void getChatMessagesFinished(const AuthResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {});
};

#endif // CHATSCONTROLLER_H
