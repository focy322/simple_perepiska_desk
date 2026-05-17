#include "listviewdragndrop.h"

ListViewDragNDrop::ListViewDragNDrop (QWidget *parent)
    : QListView(parent)
    , filePathsByChat{}
    , currentChatId(ULONG_LONG_MAX)
{
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDefaultDropAction(Qt::CopyAction);
}

QSet<QString> ListViewDragNDrop::getFilePaths(unsigned long long chatId) const
{
    return filePathsByChat.value(chatId);
}

void ListViewDragNDrop::clearFilePaths(unsigned long long chatId)
{
    filePathsByChat.remove(chatId);
}

void ListViewDragNDrop::clearAllFilePaths()
{
    filePathsByChat.clear();
}

bool ListViewDragNDrop::hasPendingFiles(unsigned long long chatId) const
{
    const auto it = filePathsByChat.constFind(chatId);
    return it != filePathsByChat.constEnd() && !it.value().isEmpty();
}

void ListViewDragNDrop::removeFileByPath(unsigned long long chatId, const QString &filePath)
{
    auto it = filePathsByChat.find(chatId);
    if (it == filePathsByChat.end())
        return;

    QSet<QString> &paths = it.value();
    paths.remove(filePath);

    if (paths.isEmpty())
        filePathsByChat.erase(it);
}

void ListViewDragNDrop::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void ListViewDragNDrop::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void ListViewDragNDrop::dropEvent(QDropEvent *event)
{
    QList<QUrl> urlList;
    if (event->mimeData()->hasUrls())
        urlList = event->mimeData()->urls();

    if (currentChatId == ULONG_LONG_MAX)
        return;

    QSet<QString> &pathsForChat = filePathsByChat[currentChatId];

    for (const QUrl &url : std::as_const(urlList))
    {
        QString filePath = url.toLocalFile();
        QFileInfo fileChecker(filePath);
        if (!filePath.isEmpty() && fileChecker.isFile())
        {
            pathsForChat.insert(filePath);
#ifdef QT_DEBUG
            qDebug() << "Файл получен через Drag-and-Drop:" << filePath;
#endif
        }
    }

    if (!pathsForChat.isEmpty())
        emit gotDragNDropFiles();
}


