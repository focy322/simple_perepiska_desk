#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H
#include "authtypes.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
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
    void registerUser(const QString &login, const QString &password, const QString &passwordConfirm);

    /**
      * Выполняет запрос на авторизацию
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @return
      */
    void logIn(const QString &login, const QString &password);

    /**
      * Пока нема

      * @return
      */
    void logOut();

private:
    QNetworkAccessManager *network;
    QString baseUrl;
    QString registerUrl;
    QString logInUrl;
signals:
    void registrationFinished(const AuthResult &res);
    void logInFinished(const AuthResult &res);
    void logOutFinished(const AuthResult &res);
};

#endif // AUTHSERVICE_H
