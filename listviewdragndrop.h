#ifndef LISTVIEWDRAGNDROP_H
#define LISTVIEWDRAGNDROP_H

#include <QListView>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QFileInfo>

class ListViewDragNDrop : public QListView
{
    Q_OBJECT

public:
    explicit ListViewDragNDrop(QWidget *parent = nullptr);
    const QStringList getFilePaths() { return filePaths; };
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QStringList filePaths;

signals:
    void gotDragNDropFiles();
};

#endif // LISTVIEWDRAGNDROP_H
