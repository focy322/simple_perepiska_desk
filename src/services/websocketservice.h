#ifndef WEBSOCKETSERVICE_H
#define WEBSOCKETSERVICE_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QWebSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils/errortypes.h"
#include "services/chatservice.h"
#include "utils/endpoints.h"

/**
 * Структура для хранения распарсенных данных о принятом сервером сообщении.
 */
struct ParsedMessageAcceptedObject
{
    QString            clientMessageId;                   //!< Временный ID на клиенте (UUID)
    QString            timestamp;                         //!< Время принятия сервером
    unsigned long long messageId = 0;                     //!< Постоянный ID сообщения на сервере
    unsigned long long chatId = 0;                        //!< Идентификатор чата
    bool               deduped = false;                   //!< Флаг дедупликации (если сообщение отправлено дважды)
};

/**
 * Сервис для управления WebSocket-соединением с сервером.
 * Отвечает за прием входящих событий в реальном времени (новые сообщения, статусы) и отправку сообщений.
 */
class WebsocketService : public QObject
{
    Q_OBJECT
public:
    using Handler = void(WebsocketService::*)(const QJsonObject& payload);

    explicit WebsocketService(QObject *parent = nullptr);

    /**
     * Устанавливает WebSocket-соединение.
     * \param accessToken токен доступа (Access Token)
     */
    void connectSocket(const QString &accessToken);

    /**
     * Разрывает текущее WebSocket-соединение.
     */
    void disconnectSocket();

    /**
     * Отправляет новое сообщение в канал WebSocket'а.
     * \param message объект сообщения для отправки
     */
    void sendMessage(const ParsedChatMessagesArrayObject &message);

    /**
     * Инициализирует таблицу обработчиков входящих событий.
     */
    void fillHandlersMap();

signals:
    // --- Сигналы процессов ---
    
    /**
     * Сигнал о начале установки соединения.
     */
    void socketConnectionInProgress();

    /**
     * Сигнал о начале разрыва соединения.
     */
    void socketDisonnectionInProgress();

    /**
     * Сигнал о начале отправки сообщения.
     */
    void sendingMessageInProgress();

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании установки соединения.
     * \param res результат выполнения запроса
     */
    void socketConnectionFinished(const NetworkResult &res);

    /**
     * Сигнал об окончании разрыва соединения.
     * \param res результат выполнения запроса
     */
    void socketDisonnectionFinished(const NetworkResult &res);

    /**
     * Сигнал об окончании отправки сообщения.
     * \param res результат выполнения запроса
     */
    void sendingMessageFinished(const NetworkResult &res);

    // --- Входящие события от сервера ---

    /**
     * Сигнал о получении нового входящего сообщения от сервера.
     * \param newMessage объект полученного сообщения
     */
    void newMessageRecieved(const ParsedChatMessagesArrayObject &newMessage);

    /**
     * Сигнал о подтверждении доставки сообщения сервером.
     * \param msgAccObj объект с информацией о принятом сообщении
     */
    void messageAccepted(const ParsedMessageAcceptedObject &msgAccObj);

    /**
     * Сигнал о том, что сообщение было прочитано собеседником.
     * \param userId идентификатор пользователя, прочитавшего сообщение
     * \param chatId идентификатор чата
     * \param lastReadMessageId идентификатор последнего прочитанного сообщения
     */
    void messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId);

private slots:
    // --- Внутренние слоты-обработчики WebSocket'а ---

    /**
     * Обработчик получения текстового сообщения (JSON) из сокета.
     * \param message текст входящего сообщения
     */
    void on_textMessageReceived(const QString &message);

private:
    // --- Внутренние объекты сети ---
    QWebSocket *           websocket;                     //!< Объект WebSocket-клиента
    
    // --- Адреса API (Endpoints) ---
    QString                baseUrl;                       //!< Базовый адрес API
    QString                webSocketUrl;                  //!< Путь API для WebSocket-соединения

    // --- Таймеры ---
    QTimer *               ackFlushTimer;                 //!< Таймер для периодической отправки Ack-сообщений
    QTimer *               outgoingMessagesFlushTimer;    //!< Таймер для переотправки потерянных сообщений
    int                    ackFlushIntervalMs;            //!< Интервал отправки Ack (в мс)
    int                    outgoingMessagesFlushIntervalMs; //!< Интервал переотправки исходящих сообщений (в мс)
    
    // --- Буферы и очереди ---
    QSet<unsigned long long> pendingDeliveryIds;          //!< Список ID сообщений, ожидающих подтверждения доставки на клиент
    QHash<QString, QJsonObject> pendingOutgoingMessages;  //!< Буфер исходящих сообщений, ожидающих подтверждения от сервера
    
    // --- Обработчики ---
    void on_newMessage(const QJsonObject &payload);       //!< Обработчик события нового сообщения
    void on_messageAccepted(const QJsonObject &payload);  //!< Обработчик подтверждения принятия сообщения
    void on_ackResult(const QJsonObject &payload);        //!< Обработчик получения квитанции (Ack)
    void on_markedRead(const QJsonObject &payload);       //!< Обработчик события прочтения сообщения
    void on_userStatus(const QJsonObject &payload);       //!< Обработчик изменения статуса пользователя
    void on_error(const QJsonObject &payload);            //!< Обработчик события ошибки от сервера
    void on_messageEdited(const QJsonObject &payload);    //!< Обработчик события редактирования сообщения
    void on_messageDeleted(const QJsonObject &payload);   //!< Обработчик события удаления сообщения
    
    QHash<QString, Handler> handlersMapByTypeOfMessage;   //!< Хэш-таблица обработчиков по типу получаемого сообщения
    
    /**
     * Отправляет накопившиеся подтверждения доставки (Acks) на сервер.
     */
    void flushPendingAcks();

    /**
     * Повторяет отправку сообщений, не получивших подтверждение от сервера.
     */
    void flushPendingOutgoingMessages();

    /**
     * Вызывает нужный обработчик из таблицы handlersMapByTypeOfMessage.
     * \param type тип события
     * \param payload данные события
     */
    void callHandler(const QString &type, const QJsonObject& payload);
};

#endif // WEBSOCKETSERVICE_H
