#ifndef LISTVIEWDRAGNDROP_H
#define LISTVIEWDRAGNDROP_H

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

private:
    QHash<unsigned long long, QSet<QString>> filePathsByChat;
    unsigned long long currentChatId = ULONG_LONG_MAX;

signals:
    void gotDragNDropFiles();
};

#endif // LISTVIEWDRAGNDROP_H
