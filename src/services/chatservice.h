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
#include "utils/endpoints.h"

/**
 * Структура для хранения распарсенных данных о чате из списка чатов.
 */
struct ParsedChatsListArrayObject
{
    QString            chatName;                          //!< Имя чата
    QString            type;                              //!< Тип чата
    QString            username;                          //!< Логин собеседника
    QString            nickname;                          //!< Никнейм собеседника
    QString            userAvatarFileUrl;                 //!< Ссылка на аватар пользователя
    QString            lastMessage;                       //!< Текст последнего сообщения
    QString            lastMessageTimestamp;              //!< Время последнего сообщения
    QString            lastMessageAttachmentType;         //!< Тип вложения последнего сообщения
    unsigned long long chatId = ULONG_LONG_MAX;           //!< Идентификатор чата
    unsigned long long chatAvatarFileId = ULONG_LONG_MAX; //!< Идентификатор аватара чата
    unsigned long long userId = ULONG_LONG_MAX;           //!< Идентификатор пользователя
    unsigned long long lastMessageId = ULONG_LONG_MAX;    //!< Идентификатор последнего сообщения
    unsigned long long lastMessageSenderId = ULONG_LONG_MAX; //!< Идентификатор отправителя последнего сообщения
    unsigned long long lastMessageAttachmentsCount = 0;   //!< Количество вложений в последнем сообщении
    unsigned int       unreadCount = 0;                   //!< Количество непрочитанных сообщений
    bool               lastMessageHasAttachments = false; //!< Наличие вложений в последнем сообщении
};

/**
 * Структура для хранения распарсенных данных об отдельном сообщении.
 */
struct ParsedChatMessagesArrayObject
{
    QString            message;                           //!< Текст сообщения
    QString            timestamp;                         //!< Временная метка отправки
    QString            clientMessageId;                   //!< Временный идентификатор на клиенте
    QString            editedAt;                          //!< Временная метка редактирования
    QString            Uuid;                              //!< UUID сообщения
    QString            readAt;                            //!< Временная метка прочтения
    QJsonArray         attachments;                       //!< Массив вложений
    unsigned long long messageId = ULONG_LONG_MAX;        //!< Идентификатор сообщения
    unsigned long long senderId = ULONG_LONG_MAX;         //!< Идентификатор отправителя
    unsigned long long chatId = ULONG_LONG_MAX;           //!< Идентификатор чата
    unsigned int       attachmentsCount = 0;              //!< Количество вложений
    bool               isPending = false;                 //!< Флаг ожидания отправки
    bool               read = false;                      //!< Флаг прочтения
    bool               edited = false;                    //!< Флаг редактирования
    bool               hasAttachments = false;            //!< Флаг наличия вложений
};

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
     */
    void getMyChats(const QString &accToken);

    /**
     * Выполняет сетевой запрос на получение истории сообщений указанного чата.
     * \param chatId идентификатор чата
     * \param accToken токен доступа (Access Token)
     */
    void getChatMessages(const unsigned long long &chatId, const QString &accToken);

    /**
     * Разбирает JSON-документ со списком чатов в вектор структур ParsedChatsListArrayObject.
     * \param doc JSON документ от API
     * \return список чатов
     */
    const std::vector<ParsedChatsListArrayObject> parseChatsListArray(const QJsonDocument &doc);

    /**
     * Разбирает JSON-документ с истории сообщений в вектор структур ParsedChatMessagesArrayObject.
     * \param doc JSON документ от API
     * \return список сообщений
     */
    const std::vector<ParsedChatMessagesArrayObject> parseChatMessagesArray(const QJsonDocument &doc);

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
     */
    void editMessage(const quint64 messageId, const quint64 chatId, const QString &newText, const QString &accToken);

    /**
     * Выполняет сетевой запрос на удаление списка сообщений.
     * \param messageIds список идентификаторов удаляемых сообщений
     * \param chatId идентификатор чата
     * \param deleteForAll удалить ли для всех или только для себя (не используется)
     * \param accToken токен доступа (Access Token)
     */
    void deleteMessage(const std::vector<quint64>& messageIds, const quint64 chatId, const bool deleteForAll, const QString &accToken);

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
     * \param paObjects полученный список чатов
     */
    void getMyChatsFinished(const NetworkResult &res, const std::vector<ParsedChatsListArrayObject>& paObjects = {} );

    /**
     * Сигнал об окончании загрузки сообщений чата.
     * \param res результат выполнения запроса
     * \param chatId идентификатор чата
     * \param paObjects полученный список сообщений
     */
    void getChatMessagesFinished(const NetworkResult &res, const unsigned long long chatId = ULONG_LONG_MAX, const std::vector<ParsedChatMessagesArrayObject>& paObjects = {} );

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
    // --- Внутренние объекты сети ---
    QNetworkAccessManager *network;                               //!< Менеджер сети для выполнения HTTP-запросов

    // --- Адреса API (Endpoints) ---
    QString                baseUrl;                               //!< Базовый адрес API
    QString                myChatsUrl;                            //!< Путь API для списка чатов
    QString                chatMessagesUrl;                       //!< Путь API для списка сообщений чата
    QString                createDirectChatUrl;                   //!< Путь API для создания личного чата
    QString                markMessageReadUrl;                    //!< Путь API для отметки сообщения прочитанным
};

#endif // CHATSERVICE_H
