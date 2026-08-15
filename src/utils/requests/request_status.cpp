#include "request_status.h"
#include <QRandomGenerator>

RequestStatusManager::RequestStatusManager(QObject *parent)
    : QObject(parent)
{
    for (size_t i = 0; i < static_cast<int>(RequestType::REQUESTS_COUNTS); ++i)
    {
        statuses_[static_cast<RequestType>(i)] = RequestState::REQUEST_IDLE;
    }
}

RequestState RequestStatusManager::getStatus(RequestType type) const
{
    return statuses_.value(type, RequestState::REQUEST_IDLE);
}

void RequestStatusManager::setStatus(RequestType type, RequestState newState)
{
    statuses_.insert(type, newState);
    emit statusChanged(type, newState);
}
