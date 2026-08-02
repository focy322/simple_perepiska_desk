#ifndef VIDEOHELPERS_H
#define VIDEOHELPERS_H

#include <QObject>
#include <QPixmap>
#include <QHash>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QUrl>

class VideoThumbnailManager : public QObject
{
    Q_OBJECT
public:
    static VideoThumbnailManager* instance();
    QPixmap getThumbnail(const QString &path);

signals:
    void thumbnailReady(const QString &path);

private:
    explicit VideoThumbnailManager(QObject *parent = nullptr);
    QHash<QString, QPixmap> m_cache;
    QHash<QString, bool> m_pending;
    
    void extractThumbnail(const QString &path);
};

class VideoPlayerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VideoPlayerDialog(const QString &path, QWidget *parent = nullptr);
    ~VideoPlayerDialog();
private:
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
};

#endif // VIDEOHELPERS_H
