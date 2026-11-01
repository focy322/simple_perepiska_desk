//
// Created by belya on 15.08.2026.
//

#include "retryable_request.h"

#include <QRandomGenerator>

uint calculateRequestDelay(int retryCount)
{
    static uint baseDelay = 1000;
    static uint maxDelay = 30000;
    uint delay = baseDelay * (1u << retryCount);
    int jitter = QRandomGenerator::global()->bounded(1000);
    delay = std::min(delay + jitter, maxDelay);

    return delay;
}

