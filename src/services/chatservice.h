#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include "utils/errortypes.h"
#include "utils/parsed_objects.h"
#include "requests/retryable_request.h"

/**
 * Сервис для взаимодействия с API чатов.
 * Отвечает за получение списков чатов, сообщений, создание новых чатов и работу с сообщениями.
 */
class ChatService : public QObject
{
    Q_OBJECT
public:
    explicit ChatService(QObject *parent = nullptr);

    /**
     * Выполняет сетевой запрос на получение списка чатов текущего пользователя.
     * \param accToken токен доступа (Access Token)
     * \param retryableReq
     * \param retryableReq
     */
    void getMyChats(const QString &accToken, RetryableRequest retryableReq);

    /**
     * Выполняет сетевой запрос на получение истории сообщений указанного чата.
     * \param chatId идентификатор чата
     * \param accToken токен доступа (Access Token)
     * \param retryableReq
     * \param myUserId
     * \param lastMsgId
     * \param lastMsgId
     * \param retryableReq
     */
    void getChatMessages(const unsigned long long &chatId, const QString &accToken, RetryableRequest retryableReq, quint64 myUserId, quint64
                         lastMsgId = 0);

    /**
     * Выполняет сетевой запрос на создание нового личного чата.
     * \param userId идентификатор собеседника
     * \param accToken токен доступа (Access Token)
     */
    void createDirectChat(const unsigned long long &userId, const QString &accToken);

    /**
     * Выполняет сетевой запрос на отметку сообщения как прочитанного.
     * Операция выполняется по принципу "fire-and-forget", сигнал о завершении не отправляется.
     * \param msg пара (ChatId, MessageId)
     * \param accToken токен доступа (Access Token)
     */
    void markMessageRead(const std::pair<quint64, quint64> &msg, const QString &accToken);

    /**
     * Выполняет сетевой запрос на редактирование сообщения.
     * \param messageId идентификатор сообщения
     * \param chatId идентификатор чата
     * \param newText новый текст сообщения
     * \param accToken токен доступа (Access Token)
     * \param reReq
     * \param reReq
     * \param reReq
     */
    void editMessage(const quint64 messageId, const quint64 chatId, const QString &newText, const QString &accToken, RetryableRequest reReq);

    /**
     * Выполняет сетевой запрос на удаление списка сообщений.
     * \param messageIds список идентификаторов удаляемых сообщений
     * \param chatId идентификатор чата
     * \param deleteForAll удалить ли для всех или только для себя (не используется)
     * \param accToken токен доступа (Access Token)
     * \param req
     * \param req
     */
    void deleteMessage(const std::vector<quint64>& messageIds, const quint64 chatId, const bool deleteForAll, const QString &accToken, RetryableRequest
                       retReq);

signals:
    // --- Сигналы процессов ---

    /**
     * Сигнал о начале загрузки списка чатов.
     */
    void getMyChatsInProgress();

    /**
     * Сигнал о начале загрузки сообщений чата.
     */
    void getChatMessagesInProgress();

    /**
     * Сигнал о начале создания личного чата.
     */
    void createDirectChatInProgress();

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании загрузки списка чатов.
     * \param res результат выполнения запроса
     * \param req
     * \param req
     * \param paObjects полученный список чатов
     */
    void getMyChatsFinished(const NetworkResult &res, RetryableRequest req, const std::vector<ParsedChatsListArrayObject>& paObjects = {});

    /**
     * Сигнал об окончании загрузки сообщений чата.
     * \param res результат выполнения запроса
     * \param req
     * \param req
     * \param chatId идентификатор чата
     * \param paObjects полученный список сообщений
     */
    void getChatMessagesFinished(const NetworkResult &res, RetryableRequest req, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {});

    /**
     * Сигнал об окончании создания личного чата.
     * \param res результат выполнения запроса
     */
    void createDirectChatFinished(const NetworkResult &res);

    /**
     * Сигнал об окончании редактирования сообщения.
     * \param res результат выполнения запроса
     * \param req информация о повторяемом запросе
     */
    void editMessageFinished(const NetworkResult &res, RetryableRequest req);

    /**
     * Сигнал об окончании удаления сообщения.
     * \param res результат выполнения запроса
     * \param req информация о повторяемом запросе
     */
    void deleteMessageFinished(const NetworkResult &res, RetryableRequest req);

private:
    // --- Внутренние объекты сети ---
    QNetworkAccessManager *network;                               //!< Менеджер сети для выполнения HTTP-запросов

};

#endif // CHATSERVICE_H
