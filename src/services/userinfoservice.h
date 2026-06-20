#ifndef USERINFOSERVICE_H
#define USERINFOSERVICE_H

#include <QObject>
#include "errortypes.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "endpoints.h"

struct ParsedFoundUsersObject
{
    QString username;   //!< Логин
    QString nickname;
    QString lastSeen;   // to DateTime
    QString avatarFileUrl;
    unsigned long long userId = ULONG_LONG_MAX;
};

// Класс для работы с информацией о пользователе получающий данные от UserInfoController и работающий с API
class UserInfoService : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoService(QObject *parent = nullptr);

    // Получить информацию о пользователе
    void getMyUserInfo(const QString &accToken);

    const std::vector<ParsedFoundUsersObject> parseFoundUsersArray(const QJsonDocument &doc);
    void findUser(const QString &accToken, const QString &input);

private:
    QNetworkAccessManager *network; //!< Указатель на объект для работы с запросами
    QString baseUrl;                //!< Базовый адрес API
    QString myUserInfoUrl;          //!< Адрес для регистрации
    QString userByUsernameUrl;      //!< Адрес для авторизации
    QString findUserUrl;
signals:

    void getMyUserInfoFinished(const NetworkResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX);
    void getMyUserInfoInProgress();
    void findUserInProgress();
    void findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {});
};

#endif // USERINFOSERVICE_H
