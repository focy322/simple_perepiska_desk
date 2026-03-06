#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>

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
{
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
    ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);// Показывать окно входа сначала
    ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);// Показывать окно входа сначала

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
    // Условия для регистрации
    bool registrationCond = (!login.isEmpty() && !password.isEmpty() && (login.size() >= 3) && (password.size() >=6) && (password == passwordConfirm));
    if (registrationCond)
    {
        ui->succesRegistrationLabel->setStyleSheet("color: green;");
        ui->succesRegistrationLabel->setText("Регистрация прошла успешно!");
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
    }
    else
    {
        ui->succesRegistrationLabel->setStyleSheet("color: red;");
        //TODO: расписать детальней все случаи
        ui->succesRegistrationLabel->setText("Ошибка регистрации");
    }

}

void MainWindow::on_logInBtn_clicked()
{
    QString login  = ui->logInLogIn->text();
    QString password = ui->logInPassword->text();
    // Условия для входа
    bool registrationCond = (!login.isEmpty() && !password.isEmpty() && (login.size() >= 3) && (password.size() >=6));
    if (registrationCond)
    {
        ui->succesLogInLabel->setStyleSheet("color: green;");
        ui->succesLogInLabel->setText("Успешный вход!");
        ui->authAndAppWidgets->setCurrentWidget(ui->pageApp);
    }
    else
    {
        ui->succesLogInLabel->setStyleSheet("color: red;");
        //TODO: расписать детальней все случаи
        ui->succesLogInLabel->setText("Ошибка входа");
    }

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

    const QRect r = this->rect(); // Габариты MainWindow

    // В левый нижний угол
    const int x = margin;
    const int y = r.height() - logOutBtn->height() - margin;

    logOutBtn->move(x, y);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionLogoutButton();
}

void MainWindow::on_logOutBtn_clicked()
{
    ui->authAndAppWidgets->setCurrentWidget(ui->pageAuth);
    ui->registrationAndLogInWidgets->setCurrentWidget(ui->pageLogIn);
}
