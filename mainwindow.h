#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>


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

private slots:
    /**
      * При выборе чата из списка чатов в messagesView (chatName) выводит его название
      *
      * @param chatIndex индекс чата из списка чатов chatsList
      */
    void on_chatsView_clicked(const QModelIndex &chatIndex);

    /**
      * При нажатии на кнопку отправки сообщения добавляет строку из messageInput в хранилище сообщений
      *
      */
    void on_sendMessageBtn_clicked();

    // TODO: нормальное описание
    /**
      * При нажатии на кнопку "Зарегистрироваться" происходит регистрация
      *
      */
    void on_registrationBtn_clicked();

    // TODO: добавить кнопку revealPassword для обоих полей с паролями на pressed() и released()

private:
    Ui::MainWindow *ui;
    QStringListModel *chatsListModel;          //!< Модель для списков чатов для отображения
    QStringListModel *messagesListModel;       //!< Модель для сообщений для конкретного чата
    QHash<QString, QStringList> chatMessages;  //!< Хранилище сообщений по чатам
    QString currentChatName;                   //!< Название текущего открытого чата

};
#endif // MAINWINDOW_H
