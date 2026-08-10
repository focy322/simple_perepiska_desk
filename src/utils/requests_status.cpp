#include "requests_status.h"
//
// Created by belya on 10.08.2026.
//
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
