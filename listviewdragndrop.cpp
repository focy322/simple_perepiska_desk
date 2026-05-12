#include "listviewdragndrop.h"

ListViewDragNDrop::ListViewDragNDrop (QWidget *parent)
    : QListView(parent),
    filePaths{}
{
    setAcceptDrops(true);
}

void ListViewDragNDrop::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void ListViewDragNDrop::dropEvent(QDropEvent *event)
{
    QList<QUrl> urlList;
    if (event->mimeData()->hasUrls())
        urlList = event->mimeData()->urls();

    for (const QUrl &url : std::as_const(urlList))
    {
        QString filePath = url.toLocalFile();
        QFileInfo fileChecker(filePath);
        if (!filePath.isEmpty() && fileChecker.isFile())
        {
            filePaths.append(filePath);
#ifdef QT_DEBUG
            qDebug() << "Файл получен через Drag-and-Drop:" << filePath;
#endif
        }
    }

    if (!filePaths.isEmpty())
        emit gotDragNDropFiles();
}


