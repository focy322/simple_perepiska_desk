#include "listviewdragndrop.h"
#include "chatmessagesitemdelegate.h"
#include "QScrollBar"

ListViewDragNDrop::ListViewDragNDrop (QWidget *parent)
    : QListView(parent)
    , filePathsByChat{}
    , currentChatId(ULONG_LONG_MAX)
    , scrollStopTimer(new QTimer(this))
    ,lastReadMessage_{ULONG_LONG_MAX, ULONG_LONG_MAX}
{
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDefaultDropAction(Qt::CopyAction);
    scrollStopTimer->setSingleShot(true);
    scrollStopTimer->setInterval(SCROLL_STOP_TIMER_INTERVAL);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](){scrollStopTimer->start();});
    connect(scrollStopTimer, &QTimer::timeout, this, &ListViewDragNDrop::on_scrollStop);
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

void ListViewDragNDrop::paintEvent(QPaintEvent *event)
{
    QListView::paintEvent(event);
    ChatMessagesItemDelegate* delegate = qobject_cast<ChatMessagesItemDelegate*>(itemDelegate());
    if (delegate)
    {
        const std::pair<quint64, quint64> lastReadMessage = delegate->getLastReadMessage();
        if (lastReadMessage.second != ULONG_LONG_MAX)
            setLastReadMessage(lastReadMessage.first, lastReadMessage.second);
    }

}

void ListViewDragNDrop::on_scrollStop()
{
    if (lastReadMessage_.second != ULONG_LONG_MAX)
        emit needReadLastMessage(lastReadMessage_);
}

void ListViewDragNDrop::setLastReadMessage(const quint64 chatId, const quint64 messageId)
{
    lastReadMessage_ = {chatId, messageId};
}



