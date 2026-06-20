#include "userinfocontroller.h"

UserInfoController::UserInfoController(QObject *parent)
    : QObject{parent}
    , userInfoService(new UserInfoService(this))
{
    // Прокидываем сигналы от UserInfoService
    connect(userInfoService, &UserInfoService::getMyUserInfoFinished, this, &UserInfoController::getMyUserInfoFinished);
    connect(userInfoService, &UserInfoService::getMyUserInfoInProgress, this, &UserInfoController::getMyUserInfoInProgress);
    connect(userInfoService, &UserInfoService::findUserInProgress, this, &UserInfoController::findUserInProgress);
    connect(userInfoService, &UserInfoService::findUserFinished, this, &UserInfoController::findUserFinished);

}


void UserInfoController::requestMyUserInfo(const QString &accToken)
{
    //TODO: может быть какую нибудь проверку на наличие токена написать здесь
    userInfoService->getMyUserInfo(accToken);
}

void UserInfoController::requestFindUser(const QString &accessToken, const QString &input)
{
    userInfoService->findUser(accessToken, input);
}
