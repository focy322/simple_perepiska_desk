#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include <QObject>
#include "authservice.h"

// Принимает запросы от UI, дергает AuthService, возвращает результат через сигналы.
class AuthController : public QObject
{
    Q_OBJECT
public:
    explicit AuthController(QObject *parent = nullptr);

    /**
      * Выполняет запрос на регисатрацию через AuthService и посылает сигнал registrationFinished
      * с AuthResult в качестве входного параметра полученным от authService
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @param passwordConfirm - Введенное подтверждение пароля
      * @return
      */
    void requestRegistration(const QString &login, const QString &password, const QString &passwordConfirm);


    /**
      * Выполняет запрос на авторизацию через AuthService и посылает сигнал logInFinished
      * с AuthResult в качестве входного параметра полученным от authService
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @return
      */
    void requestLogIn(const QString &login, const QString &password);

    /**
      * Выполняет запрос на выход из аккаунта через AuthService (Пока его нет)
      * @param
      * @return
      */
    void requestLogOut();

private:
    AuthService *authService; // Содержит правила регистрации/входа и работу с данными пользователей, без UI-зависимостей.

signals:

    void registrationFinished(const AuthResult &res);
    void logInFinished(const AuthResult &res);
    void loggedOut(const AuthResult &res);
};

#endif // AUTHCONTROLLER_H
