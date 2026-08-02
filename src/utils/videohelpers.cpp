#include "videohelpers.h"
#include <QImage>
#include <QPainter>
#include <QAudioOutput>
#include <QTimer>

VideoThumbnailManager* VideoThumbnailManager::instance()
{
    static VideoThumbnailManager manager;
    return &manager;
}

VideoThumbnailManager::VideoThumbnailManager(QObject *parent) : QObject(parent)
{
}

QPixmap VideoThumbnailManager::getThumbnail(const QString &path)
{
    if (m_cache.contains(path)) {
        return m_cache.value(path);
    }
    
    if (!m_pending.value(path, false)) {
        m_pending.insert(path, true);
        extractThumbnail(path);
    }
    
    return QPixmap();
}

void VideoThumbnailManager::extractThumbnail(const QString &path)
{
    QMediaPlayer *player = new QMediaPlayer(this);
    QVideoSink *sink = new QVideoSink(player);
    player->setVideoOutput(sink);
    
    connect(sink, &QVideoSink::videoFrameChanged, this, [this, path, player](const QVideoFrame &frame) {
        if (frame.isValid()) {
            QImage img = frame.toImage();
            if (!img.isNull()) {
                m_cache.insert(path, QPixmap::fromImage(img));
                m_pending.remove(path);
                emit thumbnailReady(path);
                
                player->stop();
                player->deleteLater();
            }
        }
    });
    
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this, path, player](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            player->play();
            player->pause(); // Just want the first frame
        } else if (status == QMediaPlayer::InvalidMedia) {
            m_pending.remove(path);
            player->deleteLater();
        }
    });
    
    player->setSource(QUrl::fromLocalFile(path));
}

VideoPlayerDialog::VideoPlayerDialog(const QString &path, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Видео");
    resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_videoWidget = new QVideoWidget(this);
    layout->addWidget(m_videoWidget);
    
    m_player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(audioOutput);
    m_player->setVideoOutput(m_videoWidget);
    
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
}

VideoPlayerDialog::~VideoPlayerDialog()
{
    m_player->stop();
}
