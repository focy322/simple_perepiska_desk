#include "requests_status.h"
#include <QRandomGenerator>
#include <cmath>
uint calculateRequestDelay(int retryCount)
{
    static uint baseDelay = 1000;
    static uint maxDelay = 30000;
    uint delay = baseDelay * static_cast<int>(std::pow(2, retryCount));
    int jitter = QRandomGenerator::global()->bounded(1000);
    delay = std::min(delay + jitter, maxDelay);

    return delay;
}

RequestsStatusManager::RequestsStatusManager(QObject *parent)
    : QObject(parent)
{
    for (size_t i = 0; i < static_cast<int>(RequestTypes::REQUESTS_COUNTS); ++i)
    {
        statuses_[static_cast<RequestTypes>(i)] = RequestState::REQUEST_IDLE;
    }
}

RequestState RequestsStatusManager::getStatus(RequestTypes type) const
{
    return statuses_.value(type, RequestState::REQUEST_IDLE);
}

void RequestsStatusManager::setStatus(RequestTypes type, RequestState newState)
{
    statuses_.insert(type, newState);
    emit statusChanged(type, newState);
}
