#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFile>
#include <QThread>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chatsListModel(new QStringListModel(this))
    , messagesListModel(new QStringListModel(this))
    , chatMessages()
    , currentChatName()
    , logOutBtn(new QPushButton("Выход", nullptr))
    , authController(new AuthController(this))
    , isAuthorized(false)
    , currentUsername("")
    , currentUserId(ULONG_LONG_MAX)
    , userInfoController(new UserInfoController(this))
    , accessToken("")
    , refreshToken("")
    , chatsController(new ChatsController(this))
    , isFirstOpen(true)
    , chatsList{}
{
    connect(authController, &AuthController::registrationFinished, this, &MainWindow::on_registrationFinished);
    connect(authController, &AuthController::logInFinished, this, &MainWindow::on_logInFinished);
    connect(authController, &AuthController::logOutFinished, this, &MainWindow::on_logOutFinished);
    connect(authController, &AuthController::registrationInProgress, this, &MainWindow::on_registrationInProgress);
    connect(authController, &AuthController::logInProgress, this, &MainWindow::on_logInProgress);
    connect(authController, &AuthController::logOutInProgress, this, &MainWindow::on_logOutInProgress);
    connect(authController, &AuthController::refreshAccessTokenInProgress, this, &MainWindow::on_refreshAccessTokenInProgress);
    connect(authController, &AuthController::refreshAccessTokenFinished, this, &MainWindow::on_refreshAccessTokenFinished);
    connect(userInfoController, &UserInfoController::getMyUserInfoInProgress, this, &MainWindow::on_getMyUserInfoInProgress);
    connect(userInfoController, &UserInfoController::getMyUserInfoFinished, this, &MainWindow::on_getMyUserInfoFinished);
    connect(chatsController, &ChatsController::getMyChatsInProgress, this, &MainWindow::on_getMyChatsInProgress);
    connect(chatsController, &ChatsController::getMyChatsFinished, this, &MainWindow::on_getMyChatsFinished);

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

    ui->chatsView->setModel(chatsListModel);
    ui->messagesView->setModel(messagesListModel);
    ui->loadingAndContentWidgets->setCurrentWidget(ui->loadingPage);

    tryAuthorize();


}

MainWindow::~MainWindow()
{
    //TODO: записывать refreshToken в файл при выходе
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

void MainWindow::tryAuthorize()
{
    QFile file("refreshToken.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // файл не открылся
        isAuthorized = false;
        isFirstOpen = false;
        checkAuthorization(AuthResult{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)}, "", "");
        return;
    }

    refreshToken = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    authController->requestRefreshAccessToken(refreshToken);
}

void MainWindow::getMyInfo()
{
    qDebug() << "Отправленный  accessToken в getMyInfo " << accessToken;
    userInfoController->requestMyUserInfo(accessToken);
}

void MainWindow::checkAuthorization(const AuthResult &res, const QString &accToken, const QString &refToken)
{
    if (res.ok)
    {
        isAuthorized = res.ok;
        accessToken = accToken;
        refreshToken = refToken;
        ui->loadingAndContentWidgets->setCurrentWidget(ui->contentPage);
        getMyInfo();
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
        getChatsList();
    }
    else
    {
        isAuthorized = res.ok;
        accessToken = accToken;
        refreshToken = refToken;
        ui->loadingAndContentWidgets->setCurrentWidget(ui->contentPage);
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);// Показывать окно входа
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);// Показывать окно входа
#ifdef QT_DEBUG
        qDebug() << "authorization Failed!!!";
#endif
    }
}

void MainWindow::getChatsList()
{
    chatsController->requestMyChats(accessToken);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionLogoutButton();
}

void MainWindow::on_logOutBtn_clicked()
{
    authController->requestLogOut(accessToken, refreshToken);
}

void MainWindow::on_refreshAccessTokenInProgress()
{

}

void MainWindow::on_refreshAccessTokenFinished(const AuthResult &res, const QString &accToken, const QString &refToken)
{
    if (isFirstOpen)
    {
        isFirstOpen = false;
        checkAuthorization(res, accToken, refToken);
    }

    if (res.ok)
    {
        accessToken = accToken;
        refreshToken = refToken;
    }
    else
    {
        isAuthorized = false;
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

void MainWindow::on_getMyChatsInProgress()
{

}

void MainWindow::on_getMyChatsFinished(const AuthResult &res, const std::vector<ParsedArrayObject>& paObjects)
{
    if (res.ok)
    {
        chatsList = paObjects;

        QStringList chatNames;
        for (const ParsedArrayObject &chat : chatsList)
        {
            QString displayName = chat.chatName.trimmed();
            chatNames.append(displayName);
        }

        chatsListModel->setStringList(chatNames);
        qDebug() << "on_getMyChatsFinished = true!!!";
    }
    else
    {
        qDebug() << "on_getMyChatsFinished = false!!!";
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
        qDebug() << "Пришедший accessToken от on_registrationFinished " << accToken;
        refreshToken = refToken;
        isAuthorized = true;
        getMyInfo();
        getChatsList();
    }
    else
    {
        isAuthorized = false;
        accessToken = accToken;
        refreshToken = refToken;
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
        getChatsList();
    }
    else
    {
        isAuthorized = false;
        accessToken = accToken;
        refreshToken = refToken;
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
        isAuthorized = false;
        ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);
        ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
    }
    else
    {
        // TODO: Сообщение об ошибке
#ifdef QT_DEBUG
        qDebug() << "LogOutError";
#endif
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
