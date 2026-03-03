#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chatsListModel(new QStringListModel(this))
    , messagesListModel(new QStringListModel(this))
    , chatMessages({{"Chat 1", {"Привет", "Это чат 1"}},
                    {"Chat 2", {"Привет", "Это чат 2"}},
                    {"Chat 3", {"Привет", "Это чат 3"}}})
    , currentChatName()
{
    ui->setupUi(this);

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
    ui->AuthAndAppWidget->setCurrentWidget(ui->pageAuth); // Показывать окно регистрации сначала
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
    QString login  = ui->login->text();
    QString password = ui->password->text();
    QString passwordConfirm = ui->passwordConfirm->text();
    // Условия для регистрации
    bool registrationCond = (!login.isEmpty() && !password.isEmpty() && (login.size() >= 3) && (password.size() >=6) && (password == passwordConfirm));
    if (registrationCond)
    {
        ui->succesRegistrationLabel->setStyleSheet("color: green;");
        ui->succesRegistrationLabel->setText("Регистрация прошла успешно!");
        ui->AuthAndAppWidget->setCurrentWidget(ui->pageApp);
    }
    else
    {
        ui->succesRegistrationLabel->setStyleSheet("color: red;");
        //TODO: расписать детальней все случаи
        ui->succesRegistrationLabel->setText("Ошибка регистрации");
    }

}

