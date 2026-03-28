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

private:
    ChatService *chatService;   //!< Дергает API-шку

signals:
    void getMyChatsInProgress();
    void getMyChatsFinished(const AuthResult &res, const std::vector<ParsedArrayObject>& paObjects = {} );
};

#endif // CHATSCONTROLLER_H
