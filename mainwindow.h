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

    void on_sendMessage_clicked();

private:
    Ui::MainWindow *ui;
    QStringListModel *chatsListModel;          //!< Модель для списков чатов для отображения
    QStringListModel *messagesListModel;       //!< Модель для сообщений для конкретного чата
    QHash<QString, QStringList> chatMessages;  //!< Хранилище сообщений по чатам
    // TODO: добавить currentChatId/currentChatName как отдельное поле класса (а не брать из текста chatName)

};
#endif // MAINWINDOW_H
