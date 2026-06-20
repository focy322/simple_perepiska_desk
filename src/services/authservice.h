#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H
#include "errortypes.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "endpoints.h"

// Содержит правила регистрации/входа и работу с данными пользователей, без UI-зависимостей.
class AuthService : public QObject
{
    Q_OBJECT
public:
    explicit AuthService(QObject *parent = nullptr);

    /**
      * Выполняет запрос на регистрацию
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @param passwordConfirm - Введенное подтверждение пароля
      * @return
      */
    void registerUser(const QString &login, const QString &password);

    /**
      * Выполняет запрос на авторизацию
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @return
      */
    void logIn(const QString &login, const QString &password);

    /**
      * Пока нема
      *
      * @return
      */
    void logOut(const QString &accToken, const QString &refToken);


    /**
      * Обновление токенов авторизации
      *
      * @return запись в токен-файлы новых токенов
      */
    void refreshAccessToken(const QString &refToken);

    void writeRefreshTokenToKeychain(QObject *parent, const QString &token);

private:
    QNetworkAccessManager *network; //!< Указатель на объект для работы с запросами
    QString baseUrl;                //!< Базовый адрес API
    QString registerUrl;            //!< Адрес для регистрации
    QString logInUrl;               //!< Адрес для авторизации
    QString refreshAccessTokenUrl;  //!< Адрес для обновления токена
    QString logOutUrl;              //!< Адрес для выхода из аккаунта
signals:
    void registrationFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = ""); //!< Сигнал о завершении регистрации (может быть как успешным так и нет)
    void logInFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");        //!< Сигнал о завершении авторизации (может быть как успешным так и нет)
    void logOutFinished(const NetworkResult &res);       //!< Сигнал о завершении выхода из аккаунта (может быть как успешным так и нет)
    void registrationInProgress();                    //!< Сигнал о том что регистрация в процессе и нужно заморозить кнопки
    void logInProgress();                             //!< Сигнал о том что авторизация в процессе и нужно заморозить кнопки
    void logOutInProgress();                          //!< Сигнал о том что выход из аккаунта в процессе и нужно заморозить кнопки
    void refreshAccessTokenInProgress();
    void refreshAccessTokenFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");
};

#endif // AUTHSERVICE_H
