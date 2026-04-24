#ifndef USERINFOSERVICE_H
#define USERINFOSERVICE_H

#include <QObject>
#include "errortypes.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "endpoints.h"

// Класс для работы с информацией о пользователе получающий данные от UserInfoController и работающий с API
class UserInfoService : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoService(QObject *parent = nullptr);

    // Получить информацию о пользователе
    void getMyUserInfo(const QString &accToken);
private:
    QNetworkAccessManager *network; //!< Указатель на объект для работы с запросами
    QString baseUrl;                //!< Базовый адрес API
    QString myUserInfoUrl;          //!< Адрес для регистрации
    QString userByUsernameUrl;      //!< Адрес для авторизации
signals:

    void getMyUserInfoFinished(const NetworkResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX);
    void getMyUserInfoInProgress();
};

#endif // USERINFOSERVICE_H
