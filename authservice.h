#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H
#include "authtypes.h"
#include <QObject>
// Содержит правила регистрации/входа и работу с данными пользователей, без UI-зависимостей.
class AuthService : public QObject
{
    Q_OBJECT
public:
    explicit AuthService(QObject *parent = nullptr);

    /**
      * Выполняет запрос на регистрацию
      * @param login - бля и так все понятно
      * @param password - бля и так все понятно
      * @return
      */
    AuthResult registerUser(const QString &login, const QString &password, const QString &passwordConfirm);

    /**
      * Выполняет запрос на авторизацию
      * @param login - бля и так все понятно
      * @param password - бля и так все понятно
      * @return
      */
    AuthResult logIn(const QString &login, const QString &password);

    /**
      * Пока нема

      * @return
      */
    AuthResult logOut();
signals:
};

#endif // AUTHSERVICE_H
