#ifndef VIDEOHELPERS_H
#define VIDEOHELPERS_H

#include <QObject>
#include <QPixmap>
#include <QHash>
#include <QDateTime>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QUrl>
#include <QCloseEvent>

class QNetworkAccessManager;

class VideoThumbnailManager : public QObject
{
    Q_OBJECT
public:
    static VideoThumbnailManager* instance();
    QPixmap getThumbnail(const QString &path, const QString &thumbnailUrl = {});

signals:
    void thumbnailReady(const QString &source);

private:
    explicit VideoThumbnailManager(QObject *parent = nullptr);
    QHash<QString, QPixmap> m_cache;
    QHash<QString, bool> m_pending;
    QHash<QString, QDateTime> m_retryAfter;
    QNetworkAccessManager *m_networkManager;
    
    void extractThumbnail(const QString &path);
    void downloadThumbnail(const QUrl &url);
};

class VideoPlayerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VideoPlayerDialog(const QString &path, QWidget *parent = nullptr);
    ~VideoPlayerDialog() override;

public slots:
    void done(int result) override;

protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void releasePlayer();

    QMediaPlayer *m_player = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
};

#endif // VIDEOHELPERS_H
