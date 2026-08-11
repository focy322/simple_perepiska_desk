//
// Created by belya on 10.08.2026.
//

#ifndef VENT_REQUESTS_STATE_H
#define VENT_REQUESTS_STATE_H
#include <QHash>
#include <qtypes.h>
#include <QObject>
#include <chrono>
#include <functional>

uint calculateRequestDelay(int retryCount);

enum class RequestState : uchar
{
    REQUEST_IDLE = 0,           //!< Запрос не выполняется
    REQUEST_IN_PROGRESS,        //!< Запрос в процессе выполнения
    REQUEST_SUCCESS,            //!< Запрос успешно выполнен
    REQUEST_FAILED              //!< Запрос завершился с ошибкой
};

enum class RequestTypes
{
    //TODO: Дополнить
    REQUEST_FIND_USER = 0,
    REQUEST_GET_MY_USER_INFO,
    REQUEST_GET_USER_INFO,
    REQUEST_REFRESH_ACCESS_TOKEN,
    REQUEST_SOCKET_CONNECT,
    REQUESTS_COUNTS
};

struct RetryableRequest
{
    RequestTypes type;
    std::function<void()> requestFunction;
    uint delay = 1000; // ms
    const std::chrono::time_point<std::chrono::system_clock> creationTime = std::chrono::system_clock::now();
};

class RequestsStatusManager final : public QObject
{
    Q_OBJECT
    QHash<RequestTypes, RequestState> statuses_;
public:
    explicit RequestsStatusManager(QObject *parent = nullptr);

    RequestState getStatus(RequestTypes type) const;
    void setStatus(RequestTypes type, RequestState newState);

    RequestsStatusManager& operator =(RequestsStatusManager const&) = delete;
    RequestsStatusManager& operator =(RequestsStatusManager&&) = delete;
    RequestsStatusManager(RequestsStatusManager const&) = delete;
    RequestsStatusManager(RequestsStatusManager&&) = delete;
    ~RequestsStatusManager() override = default;

signals:
    void statusChanged(RequestTypes type, RequestState newState);

};



#endif //VENT_REQUESTS_STATE_H
