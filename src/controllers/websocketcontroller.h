#ifndef WEBSOCKETCONTROLLER_H
#define WEBSOCKETCONTROLLER_H


#include "base_controller.h"
#include "services/websocketservice.h"

/**
 * Контроллер для работы с веб-сокетами.
 * Принимает запросы от UI, управляет соединением через WebsocketService и передает полученные события через сигналы.
 */
class WebsocketController : public BaseController
{
    Q_OBJECT
public:
    explicit WebsocketController(QObject *parent = nullptr);

    /**
     * Запрашивает установку WebSocket-соединения с сервером.
     * \param accessToken токен доступа (Access Token) для авторизации
     */
    void requestConnectSocket(const QString &accessToken);

    /**
     * Запрашивает разрыв WebSocket-соединения.
     */
    void requestDisconnectSocket();

    /**
     * Отправляет сообщение в чат через WebSocket.
     * \param message объект сообщения для отправки
     */
    void requestSendMessage(const ParsedChatMessagesArrayObject &message);

signals:
    // --- Сигналы процессов ---

    void socketConnectionInProgress();    //!< Сигнал о начале установки соединения
    void socketDisonnectionInProgress();  //!< Сигнал о начале разрыва соединения
    void sendingMessageInProgress();      //!< Сигнал о начале отправки сообщения

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
     * \param msgAccObj объект с информацией о принятом сообщении (клиентский ID и серверный ID)
     */
    void messageAccepted(const ParsedMessageAcceptedObject &msgAccObj);

    /**
     * Сигнал о том, что сообщение было прочитано собеседником.
     * \param userId идентификатор пользователя, прочитавшего сообщение
     * \param chatId идентификатор чата
     * \param lastReadMessageId идентификатор последнего прочитанного сообщения
     */
    void messageMarkedRead(const quint64 userId, const quint64 chatId, const quint64 lastReadMessageId);

private:
    // --- Внутренние сервисы ---
    WebsocketService *websocketService;   //!< Сервис для работы с WebSocket соединением
};

#endif // WEBSOCKETCONTROLLER_H
