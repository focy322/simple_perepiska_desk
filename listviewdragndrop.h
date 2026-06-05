#ifndef LISTVIEWDRAGNDROP_H
#define LISTVIEWDRAGNDROP_H

#include "qtimer.h"
#include <QListView>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QFileInfo>
#include <climits>
#include <QHash>
#include <QSet>

class ListViewDragNDrop : public QListView
{
    Q_OBJECT

public:
    explicit ListViewDragNDrop(QWidget *parent = nullptr);
    void setCurrentChatId(unsigned long long chatId) { currentChatId = chatId; }
    QSet<QString> getFilePaths(unsigned long long chatId) const;
    void clearFilePaths(unsigned long long chatId);
    void clearAllFilePaths();
    bool hasPendingFiles(unsigned long long chatId) const;
    void removeFileByPath(unsigned long long chatId, const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QHash<unsigned long long, QSet<QString>> filePathsByChat;
    unsigned long long currentChatId = ULONG_LONG_MAX;
    QTimer *scrollStopTimer;
    inline static constexpr uint SCROLL_STOP_TIMER_INTERVAL = 500;
    std::pair<quint64, quint64> lastReadMessage_;
    void on_scrollStop();
    void setLastReadMessage(const quint64 chatId, const quint64 messageId);


signals:
    void gotDragNDropFiles();
    void needReadLastMessage(const std::pair<quint64, quint64> &message);
};

#endif // LISTVIEWDRAGNDROP_H
