#ifndef USERINFOCONTROLLER_H
#define USERINFOCONTROLLER_H

#include <QObject>
#include "userinfoservice.h"

// Принимает запросы от UI, дергает UserInfoService, возвращает результат через сигналы.
class UserInfoController : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoController(QObject *parent = nullptr);

    void requestMyUserInfo(const QString &accToken);

private:
    UserInfoService *userInfoService; //!< Указатель на объект UserInfoService для работы с API


signals:

    // Прокидывает сигналы от service'а
    void getMyUserInfoInProgress();
    void getMyUserInfoFinished(const AuthResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX);
};

#endif // USERINFOCONTROLLER_H
