#include "controllers/userinfocontroller.h"

UserInfoController::UserInfoController(QObject *parent)
    : QObject{parent}
    , userInfoService(new UserInfoService(this))
{
    // Прокидываем сигналы от UserInfoService
    connect(userInfoService, &UserInfoService::getMyUserInfoFinished, this, &UserInfoController::getMyUserInfoFinished);
    connect(userInfoService, &UserInfoService::getMyUserInfoInProgress, this, &UserInfoController::getMyUserInfoInProgress);
    connect(userInfoService, &UserInfoService::findUserInProgress, this, &UserInfoController::findUserInProgress);
    connect(userInfoService, &UserInfoService::findUserFinished, this, &UserInfoController::findUserFinished);
    connect(userInfoService, &UserInfoService::getUserInfoFinished, this, &UserInfoController::getUserInfoFinished);
    connect(userInfoService, &UserInfoService::uploadAvatarFinished, this, &UserInfoController::uploadAvatarFinished);

}


void UserInfoController::requestMyUserInfo(const QString &accToken)
{
    //TODO: может быть какую нибудь проверку на наличие токена написать здесь
    userInfoService->getMyUserInfo(accToken);
}

void UserInfoController::requestFindUser(const QString &accessToken, const QString &input) const
{
    userInfoService->findUser(accessToken, input);
}

void UserInfoController::requestUserInfo(const QString &accToken, unsigned long long userId) const
{
    userInfoService->getUserInfo(accToken, userId);
}

void UserInfoController::requestUploadAvatar(const QString &accToken, const QByteArray &imageData)
{
    userInfoService->uploadAvatar(accToken, imageData);
}
