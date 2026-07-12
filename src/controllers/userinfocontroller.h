#ifndef USERINFOCONTROLLER_H
#define USERINFOCONTROLLER_H

#include <QObject>
#include "services/userinfoservice.h"

// Принимает запросы от UI, дергает UserInfoService, возвращает результат через сигналы.
class UserInfoController : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoController(QObject *parent = nullptr);

    void requestMyUserInfo(const QString &accToken);
    void requestUserInfo(const QString &accToken, unsigned long long userId);
    void requestUploadAvatar(const QString &accToken, const QByteArray &imageData);
    void requestFindUser(const QString &accessToken, const QString &input);

private:
    UserInfoService *userInfoService; //!< Указатель на объект UserInfoService для работы с API


signals:

    // Прокидывает сигналы от service'а
    void getMyUserInfoInProgress();
    void getMyUserInfoFinished(const NetworkResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX, const QString &avatarUrl = "");
    void getUserInfoFinished(const NetworkResult &res, const ParsedFoundUsersObject &user = {});
    void uploadAvatarFinished(const NetworkResult &res, const QString &avatarUrl = "");
    void findUserInProgress();
    void findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {});
};

#endif // USERINFOCONTROLLER_H
