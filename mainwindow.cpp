#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFile>
#include <fstream>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chatsListModel(new QStringListModel(this))
    , messagesListModel(new QStringListModel(this))
    , chatMessages({{"Chat 1", {"Привет", "Это чат 1"}},
                    {"Chat 2", {"Привет", "Это чат 2"}},
                    {"Chat 3", {"Привет", "Это чат 3"}}})
    , currentChatName()
    , logOutBtn(new QPushButton("Выход", nullptr))
    , authController(new AuthController(this))
    , isAuthorized(false)
    , currentUsername("")
    , currentUserId(ULONG_LONG_MAX)
    , userInfoController(new UserInfoController(this))
    , accessToken("")
    , refreshToken("")
{
    connect(authController, &AuthController::registrationFinished, this, &MainWindow::on_registrationFinished);
    connect(authController, &AuthController::logInFinished, this, &MainWindow::on_logInFinished);
    connect(authController, &AuthController::logOutFinished, this, &MainWindow::on_logOutFinished);
    connect(authController, &AuthController::registrationInProgress, this, &MainWindow::on_registrationInProgress);
    connect(authController, &AuthController::logInProgress, this, &MainWindow::on_logInProgress);
    connect(authController, &AuthController::logOutInProgress, this, &MainWindow::on_logOutInProgress);
    connect(authController, &AuthController::RefreshAccessTokenInProgress, this, &MainWindow::on_RefreshAccessTokenInProgress);
    connect(authController, &AuthController::RefreshAccessTokenFinished, this, &MainWindow::on_RefreshAccessTokenFinished);
    connect(userInfoController, &UserInfoController::getMyUserInfoInProgress, this, &MainWindow::on_getMyUserInfoInProgress);
    connect(userInfoController, &UserInfoController::getMyUserInfoFinished, this, &MainWindow::on_getMyUserInfoFinished);

    ui->setupUi(this);
    setUpLogOutBtn();

    //TODO: Эту хуйню вынести куда то в отдельный файл а может и все css стили в по файлам растаскать
    ui->chatsView->setStyleSheet(
        "QListView {"
        " background-color:#FFF8DC;"
        " border: none;"
        " outline: 0; "
        " border-right: 1px solid black;"
        " }"
        "QListView::item {"
        " min-height: 36px;"
        " padding: 8px 12px;"
        " border-radius: 6px;"
        " }"
        "QListView::item:hover {"
        " background-color:#FFE4B5;"
        " }"
        "QListView::item:selected {"
        " background-color:#F4A460;"
        " color:black;"
        " }");

    chatsListModel->setStringList({"Chat 1", "Chat 2", "Chat 3"});
    ui->chatsView->setModel(chatsListModel);
    ui->messagesView->setModel(messagesListModel);
    if (!checkAuthorization())
    {
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);// Показывать окно входа сначала
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);// Показывать окно входа сначала
    }
    else
    {
        // вытащить инфу о пользователе
        getMyInfo();
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
    }



}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_chatsView_clicked(const QModelIndex &chatIndex)
{
    if (chatIndex.isValid())
    {
        QString selectedChatName = chatIndex.data().toString(); // Название выбранного чата
        auto chatIt = chatMessages.constFind(selectedChatName); // Итератор на список сообщений (QStringList) для чата с названием chatName
        if (chatIt != chatMessages.constEnd())
        {
            currentChatName = selectedChatName;
            ui->chatName->setText(currentChatName);
            messagesListModel->setStringList(chatIt.value());
        } else
            messagesListModel->setStringList({});

#ifdef QT_DEBUG
        qDebug() << chatIndex.data();
#endif
    }
    else
    {
        // По сути он невалидным быть не может поэтому хз что тут добавить
    }
}


void MainWindow::on_sendMessageBtn_clicked()
{
    QString msgToSend = ui->messageInput->text();
    bool condToSendMsg = !msgToSend.isEmpty() && chatMessages.constFind(currentChatName) != chatMessages.constEnd(); // Условия для отправки сообщения
    if (condToSendMsg)
    {
        auto chatIt = chatMessages.find(currentChatName); // Итератор на список сообщений (QStringList) для чата с названием chatName
        chatIt.value().append(msgToSend);
        messagesListModel->setStringList(chatIt.value());
        ui->messageInput->clear();

    }
    else
    {
        ui->messageInput->clear();
        return;
    }

}


void MainWindow::on_registrationBtn_clicked()
{
    QString login  = ui->registrationLogin->text();
    QString password = ui->registrationPassword->text();
    QString passwordConfirm = ui->registrationPasswordConfirm->text();

    authController->requestRegistration(login, password, passwordConfirm);

}

void MainWindow::on_logInBtn_clicked()
{
    QString login  = ui->logInLogIn->text();
    QString password = ui->logInPassword->text();

    authController->requestLogIn(login, password);

}

void MainWindow::on_revealRegistrationPasswordBtn_pressed()
{
    ui->registrationPassword->setEchoMode(QLineEdit::EchoMode::Normal);
}


void MainWindow::on_revealRegistrationPasswordBtn_released()
{
    ui->registrationPassword->setEchoMode(QLineEdit::EchoMode::Password);
}


void MainWindow::on_revealRegistrationPasswordConfirmBtn_pressed()
{
    ui->registrationPasswordConfirm->setEchoMode(QLineEdit::EchoMode::Normal);
}


void MainWindow::on_revealRegistrationPasswordConfirmBtn_released()
{
    ui->registrationPasswordConfirm->setEchoMode(QLineEdit::EchoMode::Password);
}


void MainWindow::on_revealLogInPasswordBtn_pressed()
{
    ui->logInPassword->setEchoMode(QLineEdit::EchoMode::Normal);
}


void MainWindow::on_revealLogInPasswordBtn_released()
{
    ui->logInPassword->setEchoMode(QLineEdit::EchoMode::Password);
}


void MainWindow::on_switchToLogInBtn_clicked()
{
    ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
}


void MainWindow::on_switchToRegistrationBtn_clicked()
{
    ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageRegistration);
}

void MainWindow::setUpLogOutBtn()
{

    logOutBtn->setParent(ui->pageApp);
    logOutBtn->setFixedSize(90, 30); // Размеры кнопки (может в namespace вынести? хз)
    logOutBtn->raise(); // Поднять по Z
    logOutBtn->show();
    positionLogoutButton();
    connect(logOutBtn, &QPushButton::clicked, this, &MainWindow::on_logOutBtn_clicked);
}

void MainWindow::positionLogoutButton()
{
    const int margin = 8;

    // Габариты MainWindow (Почему то через appPage не выходит (походу геометрия до конца не формируется к моменту вызова функции))
    const QRect r = this->rect();

    // В левый нижний угол
    const int x = margin;
    const int y = r.height() - logOutBtn->height() - margin;

    logOutBtn->move(x, y);
}

bool MainWindow::checkAuthorization()
{
    QFile file("refreshToken.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // файл не открылся
        isAuthorized = false;
        return isAuthorized;
    }

    refreshToken = QString::fromUtf8(file.readAll()).trimmed();
    file.close();
    // Event loop для синхронного ожидания
    QEventLoop loop;

    // Отключаем старый обработчик на время
    disconnect(authController, &AuthController::RefreshAccessTokenFinished, this, &MainWindow::on_RefreshAccessTokenFinished);

    // Подключаем сигнал к лямбде
    auto connection = connect(authController, &AuthController::RefreshAccessTokenFinished, this, [&](const AuthResult &res, const QString &accToken, const QString &refToken) {
        isAuthorized = res.ok;
        accessToken = accToken;
        refreshToken = refToken;
        loop.quit();
    });

    authController->requestRefreshAccessToken(refreshToken);
    loop.exec(); // Ждем здесь

    disconnect(connection); // Отключаем временную связь

    // Переподключаем старый обработчик
    connect(authController, &AuthController::RefreshAccessTokenFinished, this, &MainWindow::on_RefreshAccessTokenFinished);
    return isAuthorized;
}

void MainWindow::getMyInfo()
{
    userInfoController->requestMyUserInfo(accessToken);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionLogoutButton();
}

void MainWindow::on_logOutBtn_clicked()
{
    // пока logout'а нет
    authController->requestLogOut();

}

void MainWindow::on_RefreshAccessTokenInProgress()
{

}

void MainWindow::on_RefreshAccessTokenFinished(const AuthResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        accessToken = accToken;
        refreshToken = refToken;
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "on_RefreshAccessTokenFinished = false";
#endif
    }
}

void MainWindow::on_getMyUserInfoInProgress()
{

}

void MainWindow::on_getMyUserInfoFinished(const AuthResult &res, const QString &username, unsigned long long userId)
{
    if (res.ok)
    {
        currentUsername = username;
        currentUserId = userId;
#ifdef QT_DEBUG
        qDebug() << "currentUsername" << currentUsername;
        qDebug() << "currentUserId" << currentUserId;
#endif
    }
    else
    {
#ifdef QT_DEBUG
        qDebug() << "on_getMyUserInfoFinished = false";
#endif
    }
}

void MainWindow::on_registrationFinished(const AuthResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        ui->succesRegistrationLabel->setStyleSheet("color: green;");
        ui->succesRegistrationLabel->setText(res.message);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
        ui->succesRegistrationLabel->clear();
        ui->registrationLogin->clear();
        ui->registrationPassword->clear();
        ui->registrationPasswordConfirm->clear();
        accessToken = accToken;
        refreshToken = refToken;
        isAuthorized = true;
        getMyInfo();
    }
    else
    {
        ui->succesRegistrationLabel->setStyleSheet("color: red;");
        ui->succesRegistrationLabel->setText(res.message);
    }
    ui->registrationBtn->setEnabled(true);
    ui->switchToLogInBtn->setEnabled(true);
}

void MainWindow::on_logInFinished(const AuthResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        ui->succesLogInLabel->setStyleSheet("color: green;");
        ui->succesLogInLabel->setText(res.message);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
        ui->succesLogInLabel->clear();
        ui->logInLogIn->clear();
        ui->logInPassword->clear();
        accessToken = accToken;
        refreshToken = refToken;
        isAuthorized = true;
        getMyInfo();
    }
    else
    {
        ui->succesLogInLabel->setStyleSheet("color: red;");
        ui->succesLogInLabel->setText(res.message);
        ui->chatName->setText("Выберите чат");
        currentChatName = "";
    }
    ui->logInBtn->setEnabled(true);
    ui->switchToRegistrationBtn->setEnabled(true);
}

void MainWindow::on_logOutFinished(const AuthResult &res)
{
    if (res.ok)
    {
        // TODO: Очистить пользовательские данные и поля
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
    }
    else
    {
        // TODO: Сообщение об ошибке
    }
}

void MainWindow::on_registrationInProgress()
{
    // Заморозка кнопок на время регистрации
    ui->registrationBtn->setEnabled(false);
    ui->switchToLogInBtn->setEnabled(false);
}

void MainWindow::on_logInProgress()
{
    // Заморозка кнопок на время авторизации
    ui->logInBtn->setEnabled(false);
    ui->switchToRegistrationBtn->setEnabled(false);
}

void MainWindow::on_logOutInProgress()
{
    //TODO: Реализация
}
