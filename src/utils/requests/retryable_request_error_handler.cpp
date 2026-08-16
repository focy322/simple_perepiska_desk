//
// Created by belya on 15.08.2026.
//

#include "retryable_request_error_handler.h"

#include <QRandomGenerator>

RetryableRequestErrorHandler::RetryableRequestErrorHandler(QObject* parent)
    : QObject(parent)
    , pendingRetryableUnauthorizeRequests{}
    , pendingRetryableRequests{}
    , retryableRequestsTimer(new QTimer(this))
{
    retryableRequestsTimer->setInterval(500);
    retryableRequestsTimer->setSingleShot(false);
    connect(retryableRequestsTimer, &QTimer::timeout, this, [this]()
    {
        if (pendingRetryableRequests.empty())
        {
            retryableRequestsTimer->stop();
            return;
        }

        const auto now = std::chrono::system_clock::now();
        for (auto it = pendingRetryableRequests.begin(); it != pendingRetryableRequests.end();)
        {
            const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->creationTime).count();
            if (elapsedMs >= static_cast<long long>(it->delay))
            {
                it->requestFunction(*it);
                it = pendingRetryableRequests.erase(it);
            }
            else
                ++it;
        }

        if (pendingRetryableRequests.empty())
            retryableRequestsTimer->stop();
    });
}

void RetryableRequestErrorHandler::eraseRetryableRequestsByType(RequestType type, uchar option)
{
    if (option == 2)
    {
        pendingRetryableUnauthorizeRequests.remove_if(
            [type](const RetryableRequest &request)
            {
                return request.type == type;
            });

        pendingRetryableRequests.remove_if(
            [type](const RetryableRequest &request)
            {
                return request.type == type;
            });
    }
    else if (option == 1)
    {
        pendingRetryableRequests.remove_if(
            [type](const RetryableRequest &request)
            {
                return request.type == type;
            });
    }
    else if (option == 0)
    {
        pendingRetryableUnauthorizeRequests.remove_if(
            [type](const RetryableRequest &request)
            {
                return request.type == type;
            });
    }
}

void RetryableRequestErrorHandler::checkRetryableUnauthorizeRequests()
{
    std::list<RetryableRequest> requestsToProcess;
    requestsToProcess.swap(pendingRetryableUnauthorizeRequests);

    for (const auto &req : requestsToProcess)
    {
        req.requestFunction(req);
    }
}

void RetryableRequestErrorHandler::getReady(std::vector<BaseController*> newControllers, RequestStatusManager *statusManager)
{
    this->controllers = newControllers;
    this->requestsStatusManager = statusManager;
    for (const auto &controller : controllers)
    {
        connect(controller, &BaseController::errorOccurred, this, &RetryableRequestErrorHandler::handleRequestError);
    }
}

void RetryableRequestErrorHandler::clearRetryableRequests(uchar option)
{
    if (option == 2)
    {
        pendingRetryableUnauthorizeRequests.clear();
        pendingRetryableRequests.clear();
    }
    else if (option == 1)
        pendingRetryableRequests.clear();
    else if (option == 0)
        pendingRetryableUnauthorizeRequests.clear();

}

void RetryableRequestErrorHandler::addRetryableRequest(RetryableRequest request, uchar option)
{
    if (option == 1)
    {
        if (request.isReplaceable)
            eraseRetryableRequestsByType(request.type, 1);
        pendingRetryableRequests.emplace_back(request);
        if (retryableRequestsTimer->isActive() == false)
            retryableRequestsTimer->start();
    }
    else if (option == 0)
    {
        if (request.isReplaceable)
            eraseRetryableRequestsByType(request.type, 0);
        pendingRetryableUnauthorizeRequests.emplace_back(request);
    }
}

void RetryableRequestErrorHandler::handleRequestError(const NetworkResult& res, RetryableRequest request)
{
    if (res.error == ERROR_TYPES::UNAUTHORIZED && request.type == RequestType::REQUEST_REFRESH_ACCESS_TOKEN)
        return;

    request.creationTime = std::chrono::system_clock::now();
    request.delay = calculateRequestDelay(request.retryCount);
    if (res.error == ERROR_TYPES::UNAUTHORIZED)
    {
        if (request.isReplaceable)
            eraseRetryableRequestsByType(request.type, 0);
        pendingRetryableUnauthorizeRequests.emplace_back(request);

        if (requestsStatusManager->getStatus(RequestType::REQUEST_REFRESH_ACCESS_TOKEN) != RequestState::REQUEST_IN_PROGRESS)
            emit needRefreshToken();
    }
    else if (request.type != RequestType::REQUEST_SOCKET_CONNECT && request.retryCount <= RetryableRequest::maxRetryCount)
    {
        if (request.isReplaceable)
            eraseRetryableRequestsByType(request.type, 1);
        pendingRetryableRequests.emplace_back(request);
        if (!retryableRequestsTimer->isActive())
            retryableRequestsTimer->start();
    }
    else if (request.type == RequestType::REQUEST_SOCKET_CONNECT)
    {
        pendingRetryableRequests.emplace_back(request);
        if (!retryableRequestsTimer->isActive())
            retryableRequestsTimer->start();
    }
}
