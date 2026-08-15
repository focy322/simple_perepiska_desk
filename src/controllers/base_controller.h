//
// Created by belya on 15.08.2026.
//

#ifndef VENT_BASE_CONTROLLER_H
#define VENT_BASE_CONTROLLER_H
#include <QObject>

#include "utils/errortypes.h"
#include "utils/requests/retryable_request.h"

class BaseController : public QObject
{
    Q_OBJECT
protected:
    explicit BaseController(QObject* parent = nullptr) : QObject(parent) {}

public:
    ~BaseController() override = default;

    signals:
    void errorOccurred(const NetworkResult &res, RetryableRequest request);

};

#endif //VENT_BASE_CONTROLLER_H
