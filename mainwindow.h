#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QPushButton>
#include <QMainWindow>
#include <QStringListModel>
#include "authcontroller.h"
#include "userinfocontroller.h"
#include "chatscontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:

    /**
      * При событии resize у MainWindow вызывается resizeEvent базово класса + функция positionLogoutButton
      * для изменения позиции конопки "Выход"
      * @param event объект QResizeEvent
      */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
      * При выборе чата из списка чатов в messagesView (chatName) выводит его название
      *
      * @param chatItem объект представляющий нажатый элемент из chatsList
      */
    void on_chatsView_clicked(const QModelIndex &chatItem);

    /**
      * При нажатии на кнопку отправки сообщения добавляет строку из messageInput в хранилище сообщений
      * и обновляет messagesListModel обновленным хранилищем сообщений
      */
    void on_sendMessageBtn_clicked();

    // TODO: нормальное описание
    /**
      * При нажатии на кнопку "Зарегистрироваться" происходит регистрация
      *
      */
    void on_registrationBtn_clicked();

    // TODO: нормальное описание
    /**
      * При нажатии на кнопку "Войти" происходит вход
      *
      */
    void on_logInBtn_clicked();

    void on_revealRegistrationPasswordBtn_pressed();

    void on_revealRegistrationPasswordBtn_released();

    void on_revealRegistrationPasswordConfirmBtn_pressed();

    void on_revealRegistrationPasswordConfirmBtn_released();

    void on_revealLogInPasswordBtn_pressed();

    void on_revealLogInPasswordBtn_released();

    void on_switchToLogInBtn_clicked();

    void on_switchToRegistrationBtn_clicked();

    /**
      * Вызывается при получении сигнала о завершении регистрации
      * При успехе перебрасывает на основное окно приложения попутно очищая поля ввода логина и пароля
      * При неудаче выводит сообщение об ошибке
      *
      */
    void on_registrationFinished(const AuthResult &res, const QString &accToken, const QString &refToken);

    /**
      * Вызывается при получении сигнала о завершении авторизации
      * При успехе перебрасывает на основное окно приложения попутно очищая поля ввода логина и пароля
      * При неудаче выводит сообщение об ошибке
      *
      */
    void on_logInFinished(const AuthResult &res, const QString &accToken, const QString &refToken);

    /**
      * Вызывается при получении сигнала о завершении выхода из аккаунта
      * При успехе перебрасывает на окно регистрации попутно подчищая за собой все пользовательские данные(чаты, названия чатов и т.д)
      * При неудаче пока хз что
      *
      */
    void on_logOutFinished(const AuthResult &res);

    /**
      * Вызывается при получении сигнала о том что начался процесс регистрации
      * Замораживает кнопки на веремя регистрации
      *
      */
    void on_registrationInProgress();

    /**
      * Вызывается при получении сигнала о том что начался процесс авторизации
      * Замораживает кнопки на время регистрации
      *
      */
    void on_logInProgress();

    /**
      * Вызывается при получении сигнала о том что начался процесс выходп из аккаунта
      * Что то происходит
      *
      */
    void on_logOutInProgress();

    void on_logOutBtn_clicked();

    void on_refreshAccessTokenInProgress();

    void on_refreshAccessTokenFinished(const AuthResult &res, const QString &accToken, const QString &refToken);

    void on_getMyUserInfoInProgress();

    void on_getMyUserInfoFinished(const AuthResult &res, const QString &username, unsigned long long userId);

    void on_getMyChatsInProgress();

    void on_getMyChatsFinished(const AuthResult &res, const std::vector<ParsedArrayObject>& paObjects );

private:
    Ui::MainWindow *ui;
    QStringListModel *chatsListModel;          //!< Модель для списков чатов для отображения
    QStringListModel *messagesListModel;       //!< Модель для сообщений для конкретного чата
    QHash<QString, QStringList> chatMessages;  //!< Хранилище сообщений по чатам
    QString currentChatName;                   //!< Название текущего открытого чата
    QPushButton *logOutBtn;                    //!< Кнопка выхода из аккаунта
    AuthController *authController;            //!< Принимает запросы от UI, дергает AuthService, возвращает результат через сигналы.
    bool isAuthorized;                         //!< Флаг авторизации пользователя
    QString currentUsername;                   //!< Имя пользователя
    unsigned long long currentUserId;          //!< Id пользователя
    UserInfoController *userInfoController;    //!< Принимает запросы от UI, дергает UserInfoService, возвращает результат через сигналы.
    QString accessToken;
    QString refreshToken;
    ChatsController *chatsController;          //!< Принимает запросы от UI, дергает ChatService, возвращает результат через сигналы.
    bool isFirstOpen;                          //!< Флаг первого открытия приложения для авторизирования пользователя в приложение
    std::vector<ParsedArrayObject> chatsList;  //!< Список чатов состоящий из ParsedArrayObject

    /**
      * Инициализация кнопки "Выход"
      *
      */
    void setUpLogOutBtn();

    /**
      * При resiz'е окна меняет положение кнопки "Выход"
      *
      */
    void positionLogoutButton();

    void tryAuthorize();

    void getMyInfo();

    void checkAuthorization(const AuthResult &res, const QString &accToken, const QString &refToken);

    void getChatsList();
};
#endif // MAINWINDOW_H
