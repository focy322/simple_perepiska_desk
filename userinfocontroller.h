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
    void requestFindUser(const QString &accessToken, const QString &input);

private:
    UserInfoService *userInfoService; //!< Указатель на объект UserInfoService для работы с API


signals:

    // Прокидывает сигналы от service'а
    void getMyUserInfoInProgress();
    void getMyUserInfoFinished(const NetworkResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX);
    void findUserInProgress();
    void findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {});
};

#endif // USERINFOCONTROLLER_H
