//
// Created by belya on 10.08.2026.
//

#ifndef VENT_REQUESTS_STATE_H
#define VENT_REQUESTS_STATE_H
#include <QHash>
#include <qtypes.h>
#include <QObject>

enum class RequestState : uchar
{
    REQUEST_IDLE = 0,           //!< Запрос не выполняется
    REQUEST_IN_PROGRESS,        //!< Запрос в процессе выполнения
    REQUEST_SUCCESS,            //!< Запрос успешно выполнен
    REQUEST_FAILED              //!< Запрос завершился с ошибкой
};

enum class RequestType
{
    //TODO: Дополнить
    REQUEST_FIND_USER = 0,
    REQUEST_GET_MY_USER_INFO,
    REQUEST_GET_USER_INFO,
    REQUEST_REFRESH_ACCESS_TOKEN,
    REQUEST_SOCKET_CONNECT,
    REQUEST_MY_CHATS,
    REQUEST_CHAT_MESSAGES,
    REQUEST_UPLOAD_AVATAR,
    REQUESTS_COUNTS
};

class RequestStatusManager final : public QObject
{
    Q_OBJECT
    QHash<RequestType, RequestState> statuses_;
public:
    explicit RequestStatusManager(QObject *parent = nullptr);

    RequestState getStatus(RequestType type) const;
    void setStatus(RequestType type, RequestState newState);

    RequestStatusManager& operator =(RequestStatusManager const&) = delete;
    RequestStatusManager& operator =(RequestStatusManager&&) = delete;
    RequestStatusManager(RequestStatusManager const&) = delete;
    RequestStatusManager(RequestStatusManager&&) = delete;
    ~RequestStatusManager() override = default;

signals:
    void statusChanged(RequestType type, RequestState newState);

};



#endif //VENT_REQUESTS_STATE_H
