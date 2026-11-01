#include "videohelpers.h"
#include <QImage>
#include <QPainter>
#include <QAudioOutput>
#include <QTimer>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <utility>

VideoThumbnailManager* VideoThumbnailManager::instance()
{
    static VideoThumbnailManager manager;
    return &manager;
}

VideoThumbnailManager::VideoThumbnailManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

QPixmap VideoThumbnailManager::getThumbnail(const QString &path, const QString &thumbnailUrl)
{
    if (!thumbnailUrl.trimmed().isEmpty()) {
        const QUrl url(thumbnailUrl.trimmed());
        if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https")) {
            const QString key = url.toString(QUrl::FullyEncoded);
            if (m_cache.contains(key))
                return m_cache.value(key);

            if (!m_retryAfter.contains(key) || QDateTime::currentDateTimeUtc() >= m_retryAfter.value(key)) {
                if (!m_pending.value(key, false)) {
                    m_pending.insert(key, true);
                    downloadThumbnail(url);
                }
                return {};
            }
        }
    }

    if (path.isEmpty() || !QFileInfo(path).isFile())
        return {};
    if (m_cache.contains(path)) {
        return m_cache.value(path);
    }
    
    if (!m_pending.value(path, false)) {
        m_pending.insert(path, true);
        extractThumbnail(path);
    }
    
    return QPixmap();
}

void VideoThumbnailManager::downloadThumbnail(const QUrl &url)
{
    const QString key = url.toString(QUrl::FullyEncoded);
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, key]() {
        m_pending.remove(key);
        QPixmap thumbnail;
        if (reply->error() == QNetworkReply::NoError)
            thumbnail.loadFromData(reply->readAll());

        if (!thumbnail.isNull()) {
            m_cache.insert(key, thumbnail);
            m_retryAfter.remove(key);
        } else {
            // Не повторяем неудачный запрос на каждой перерисовке списка.
            m_retryAfter.insert(key, QDateTime::currentDateTimeUtc().addSecs(30));
        }
        reply->deleteLater();
        emit thumbnailReady(key);
    });
}

void VideoThumbnailManager::extractThumbnail(const QString &path)
{
    QMediaPlayer *player = new QMediaPlayer(this);
    QVideoSink *sink = new QVideoSink(player);
    player->setVideoOutput(sink);
    
    connect(sink, &QVideoSink::videoFrameChanged, this, [this, path, player](const QVideoFrame &frame) {
        if (frame.isValid())
        {
            QImage img = frame.toImage();
            if (!img.isNull()) {
                m_cache.insert(path, QPixmap::fromImage(img));
                m_pending.remove(path);
                emit thumbnailReady(path);

                player->pause();
                player->stop();
                player->deleteLater();
            }
        }
    });
    
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this, path, player](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia)
            player->play();
        else if (status == QMediaPlayer::InvalidMedia)
        {
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
    setAttribute(Qt::WA_DeleteOnClose);
    resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_videoWidget = new QVideoWidget(this);
    layout->addWidget(m_videoWidget);
    
    m_player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(m_player);
    m_player->setAudioOutput(audioOutput);
    m_player->setVideoOutput(m_videoWidget);
    
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
}

VideoPlayerDialog::~VideoPlayerDialog()
{
    releasePlayer();
}

void VideoPlayerDialog::releasePlayer()
{
    QMediaPlayer *player = std::exchange(m_player, nullptr);
    if (!player)
        return;

    // Освобождаем кадр и плеер, пока QVideoWidget и его нативное окно ещё существуют.
    player->blockSignals(true);
    player->setVideoOutput(nullptr);
    if (m_videoWidget)
        m_videoWidget->videoSink()->setVideoFrame(QVideoFrame());
    player->stop();
    delete player;
}

void VideoPlayerDialog::done(int result)
{
    releasePlayer();
    QDialog::done(result);
}

void VideoPlayerDialog::closeEvent(QCloseEvent *event)
{
    releasePlayer();
    QDialog::closeEvent(event);
}
