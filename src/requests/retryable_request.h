//
// Created by belya on 15.08.2026.
//

#ifndef VENT_RETRYABLE_REQUEST_H
#define VENT_RETRYABLE_REQUEST_H
#include <qtypes.h>

#include "request_status.h"

struct RetryableRequest
{
    RequestType type;
    std::function<void(RetryableRequest)> requestFunction;
    int retryCount = 0;
    uint delay = 1000; // ms
    std::chrono::time_point<std::chrono::system_clock> creationTime = std::chrono::system_clock::now();
    bool isReplaceable = true; // Если true, то при добавлении нового запроса того же типа, старый будет удален
    static inline int maxRetryCount = 5; // Максимальное количество повторных попыток
};

uint calculateRequestDelay(int retryCount);

#endif //VENT_RETRYABLE_REQUEST_H
