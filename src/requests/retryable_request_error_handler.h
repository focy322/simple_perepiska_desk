//
// Created by belya on 15.08.2026.
//

#ifndef VENT_REQUEST_ERROR_HANDLER_H
#define VENT_REQUEST_ERROR_HANDLER_H
#include <functional>
#include <qtypes.h>
#include <QTimer>

#include "request_status.h"
#include "retryable_request.h"
#include "controllers/base_controller.h"


class RetryableRequestErrorHandler : public QObject
{
    Q_OBJECT
    std::list<RetryableRequest> pendingRetryableUnauthorizeRequests;          //!< Очередь запросов ожидающих повторной отправки из за неавторизованного состояния (401 Unauthorized)
    std::list<RetryableRequest> pendingRetryableRequests;                     //!< Очередь запросов ожидающих повторной отправки не 401
    QTimer *retryableRequestsTimer;                                           //!< Таймер для обработки повторных запросов
    std::vector<BaseController*> controllers;                                 //!< Контроллеры, которые будут уведомлять оь ошибках
    RequestStatusManager *requestsStatusManager;                              //!< Менеджер статусов запросов

public:
    explicit RetryableRequestErrorHandler(QObject *parent = nullptr);
    ~RetryableRequestErrorHandler() override = default;

/**
 *
 * @param type
 * @param option 0 - удалить у pendingRetryableUnauthorizeRequests, 1 - удалить у pendingRetryableRequests, 2 - удалить у обоих
 */
    void eraseRetryableRequestsByType(RequestType type, uchar option);

    void checkRetryableUnauthorizeRequests();

    void getReady(std::vector<BaseController*> newControllers, RequestStatusManager *statusManager);

    /**
     *
     * @param option 0 - удалить у pendingRetryableUnauthorizeRequests, 1 - удалить у pendingRetryableRequests, 2 - удалить у обоих
     */
    void clearRetryableRequests(uchar option);

    void addRetryableRequest(RetryableRequest request, uchar option);

private slots:
    void handleRequestError(const NetworkResult &res, RetryableRequest request);

signals:
    void needRefreshToken();
    void needImmediateLogOut();
};

#endif //VENT_REQUEST_ERROR_HANDLER_H
