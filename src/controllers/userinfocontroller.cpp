#include "controllers/userinfocontroller.h"

UserInfoController::UserInfoController(QObject *parent)
    : BaseController{parent}
    , userInfoService(new UserInfoService(this))
{
    // Прокидываем сигналы от UserInfoService
    connect(userInfoService, &UserInfoService::getMyUserInfoFinished, this, &UserInfoController::on_GetMyUserInfoFinished);
    connect(userInfoService, &UserInfoService::getMyUserInfoInProgress, this, &UserInfoController::getMyUserInfoInProgress);
    connect(userInfoService, &UserInfoService::findUserInProgress, this, &UserInfoController::findUserInProgress);
    connect(userInfoService, &UserInfoService::findUserFinished, this, &UserInfoController::on_FindUserFinished);
    connect(userInfoService, &UserInfoService::getUserInfoFinished, this, &UserInfoController::on_GetUserInfoFinished);
    connect(userInfoService, &UserInfoService::uploadAvatarFinished, this, &UserInfoController::on_UploadAvatarFinished);

}


void UserInfoController::requestMyUserInfo(const QString &accToken, RetryableRequest reReq)
{
    userInfoService->getMyUserInfo(accToken, reReq);
}

void UserInfoController::requestFindUser(const QString &accessToken, const QString &arg) const
{
    userInfoService->findUser(accessToken, arg);
}

void UserInfoController::on_GetMyUserInfoFinished(const NetworkResult& res, RetryableRequest reReq,
                                                  const QString& username, unsigned long long userId, const QString& avatarUrl)
{
    if (!res.ok)
    {
        static int errorCount = 0;
        ++errorCount;
        reReq.retryCount = errorCount;
        emit errorOccurred(res, reReq);
    }
    emit getMyUserInfoFinished(res, username, userId, avatarUrl);
}

void UserInfoController::on_GetUserInfoFinished(const NetworkResult& res, RetryableRequest reReq, const ParsedFoundUsersObject& user)
{
    if (!res.ok)
    {
        static int errorCount = 0;
        ++errorCount;
        reReq.retryCount = errorCount;
        emit errorOccurred(res, reReq);
    }
    emit getUserInfoFinished(res, user);
}

void UserInfoController::on_UploadAvatarFinished(const NetworkResult& res, RetryableRequest reReq, const QString& avatarUrl)
{
    if (!res.ok)
    {
        static int errorCount = 0;
        ++errorCount;
        reReq.retryCount = errorCount;
        emit errorOccurred(res, reReq);
    }
    emit uploadAvatarFinished(res, avatarUrl);
}

void UserInfoController::on_FindUserFinished(const NetworkResult& res,
    const std::vector<ParsedFoundUsersObject>& paObjects, const QString& input)
{
    emit findUserFinished(res, paObjects, input);
}

void UserInfoController::requestUserInfo(const QString &accToken, unsigned long long userId, RetryableRequest reReq) const
{
    userInfoService->getUserInfo(accToken, userId, reReq);
}

void UserInfoController::requestUploadAvatar(const QString &accToken, const QByteArray &imageData, RetryableRequest reReq)
{
    userInfoService->uploadAvatar(accToken, imageData, reReq);
}
