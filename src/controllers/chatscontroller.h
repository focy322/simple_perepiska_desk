#ifndef CHATSCONTROLLER_H
#define CHATSCONTROLLER_H

#include <QObject>
#include "services/chatservice.h"

/**
 * Контроллер для работы с чатами и сообщениями.
 * Принимает запросы от UI, вызывает методы ChatService и возвращает результат через сигналы.
 */
class ChatsController : public QObject
{
    Q_OBJECT
public:
    explicit ChatsController(QObject *parent = nullptr);

    /**
     * Запрашивает список чатов текущего пользователя.
     * \param accToken токен доступа (Access Token) для авторизации запроса
     */
    void requestMyChats(const QString &accToken);

    /**
     * Запрашивает историю сообщений для указанного чата.
     * \param chatId идентификатор чата
     * \param accToken токен доступа (Access Token) для авторизации запроса
     */
    void requestChatMessages(const unsigned long long &chatId, const QString &accToken);

    /**
     * Создает новый личный чат с указанным пользователем.
     * \param userId идентификатор собеседника
     * \param accToken токен доступа (Access Token) для авторизации запроса
     */
    void requestCreateDirectChat(const unsigned long long &userId, const QString &accToken);

    /**
     * Отправляет запрос на отметку сообщения как прочитанного.
     * \param msg пара из идентификатора чата и идентификатора сообщения
     * \param accToken токен доступа (Access Token) для авторизации запроса
     */
    void requestMarkMessageRead(const std::pair<quint64, quint64> &msg, const QString &accToken);

    /**
     * Отправляет запрос на редактирование существующего сообщения.
     * \param messageId идентификатор сообщения
     * \param chatId идентификатор чата
     * \param newText новый текст сообщения
     * \param accToken токен доступа (Access Token) для авторизации запроса
     */
    void requestEditMessage(const quint64 messageId, const quint64 chatId, const QString &newText, const QString &accToken);

    /**
     * Отправляет запрос на удаление сообщений.
     * \param messageIds список идентификаторов удаляемых сообщений
     * \param chatId идентификатор чата
     * \param deleteForAll флаг удаления сообщения для всех участников (true) или только для себя (false)
     * \param accToken токен доступа (Access Token) для авторизации запроса
     */
    void requestDeleteMessage(const std::vector<quint64>& messageIds, const quint64 chatId, const bool deleteForAll, const QString &accToken);

signals:
    // --- Сигналы процессов ---
    
    void getMyChatsInProgress();          //!< Сигнал о начале загрузки списка чатов
    void getChatMessagesInProgress();     //!< Сигнал о начале загрузки сообщений чата
    void createDirectChatInProgress();    //!< Сигнал о начале создания личного чата
    
    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании загрузки списка чатов.
     * \param res результат выполнения запроса
     * \param paObjects полученный список чатов
     */
    void getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );

    /**
     * Сигнал об окончании загрузки сообщений чата.
     * \param res результат выполнения запроса
     * \param chatId идентификатор чата, для которого загружались сообщения
     * \param paObjects полученный список сообщений
     */
    void getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {});

    /**
     * Сигнал об окончании создания личного чата.
     * \param res результат выполнения запроса
     */
    void createDirectChatFinished(const NetworkResult &res);

    /**
     * Сигнал об окончании редактирования сообщения.
     * \param res результат выполнения запроса
     */
    void editMessageFinished(const NetworkResult &res);

    /**
     * Сигнал об окончании удаления сообщения.
     * \param res результат выполнения запроса
     */
    void deleteMessageFinished(const NetworkResult &res);

private:
    // --- Внутренние сервисы ---
    ChatService *chatService;             //!< Сервис для работы с API чатов
};

#endif // CHATSCONTROLLER_H
