#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QPushButton>
#include <QMainWindow>
#include <QStringListModel>
#include "authcontroller.h"

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
      * @param chatIndex объект представляющий нажатый элемент из chatsList
      */
    void on_chatsView_clicked(const QModelIndex &chatIndex);

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

    void on_logOutBtn_clicked();


    void on_switchToLogInBtn_clicked();

    void on_switchToRegistrationBtn_clicked();


private:
    Ui::MainWindow *ui;
    QStringListModel *chatsListModel;          //!< Модель для списков чатов для отображения
    QStringListModel *messagesListModel;       //!< Модель для сообщений для конкретного чата
    QHash<QString, QStringList> chatMessages;  //!< Хранилище сообщений по чатам
    QString currentChatName;                   //!< Название текущего открытого чата
    QPushButton *logOutBtn;                    //!< Кнопка выхода из аккаунта
    AuthController *authController;            //!< Принимает запросы от UI, дергает AuthService, возвращает результат через сигналы.

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

};
#endif // MAINWINDOW_H
