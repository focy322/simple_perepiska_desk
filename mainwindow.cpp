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
{
    ui->setupUi(this);
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
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_chatsView_clicked(const QModelIndex &chatIndex)
{
    if (chatIndex.isValid())
    {
        QString chatName = chatIndex.data().toString(); // Название чата
        ui->chatName->setText(chatName);
        auto chatIt = chatMessages.constFind(chatName); // Итератор на список сообщений (QStringList) для чата с названием chatName
        if (chatIt != chatMessages.constEnd()) {
            messagesListModel->setStringList(chatIt.value());
        } else {
            messagesListModel->setStringList({});
        }
#ifdef QT_DEBUG
        qDebug() << chatIndex.data();
#endif
    }
}


void MainWindow::on_sendMessage_clicked()
{
    QString msgToSend = ui->messageInput->text();
    bool condToSendMsg = !msgToSend.isEmpty() && chatMessages.constFind(ui->chatName->text()) != chatMessages.constEnd(); // Условия для отправки сообщения
    if (condToSendMsg)
    {
        auto chatIt = chatMessages.find(ui->chatName->text()); // Итератор на список сообщений (QStringList) для чата с названием chatName
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

