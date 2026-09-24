#include "stagingimagelabel.h"

#include <QAbstractItemView>
#include <QAbstractButton>
#include <QColor>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QVariantAnimation>

PreviewOverlayButton::PreviewOverlayButton(const QColor &defaultColor, const QColor &hoverBgColor, QWidget *parent)
    : QPushButton(parent)
    , m_defaultColor(defaultColor)
    , m_hoverBgColor(hoverBgColor)
{
    setFixedSize(18, 18);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("QPushButton { background: transparent; border: none; }");
}

void PreviewOverlayButton::setAlpha(int alpha)
{
    if (m_alpha != alpha) {
        m_alpha = alpha;
        update();
    }
}

void PreviewOverlayButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    if (m_alpha <= 0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (m_isHovered) {
        QColor color = m_hoverBgColor;
        color.setAlpha(color.alpha() * m_alpha / 255);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect());
    }

    QColor color = m_defaultColor;
    color.setAlpha(color.alpha() * m_alpha / 255);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(4, 4, 10, 10);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void PreviewOverlayButton::enterEvent(QEnterEvent *event)
#else
void PreviewOverlayButton::enterEvent(QEvent *event)
#endif
{
    m_isHovered = true;
    update();
    QPushButton::enterEvent(event);
}

void PreviewOverlayButton::leaveEvent(QEvent *event)
{
    m_isHovered = false;
    update();
    QPushButton::leaveEvent(event);
}

OverlayContainer::OverlayContainer(QWidget *parent) : QLabel(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

int OverlayContainer::alpha() const
{
    return m_alpha;
}

void OverlayContainer::setAlpha(int alpha)
{
    if (m_alpha != alpha) {
        m_alpha = alpha;
        update();
    }
}

void OverlayContainer::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (m_alpha <= 0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(36, 42, 49, 204 * m_alpha / 255));
    painter.drawRoundedRect(rect(), 12, 12);
}

StagingImageLabel::StagingImageLabel(QWidget *parent) : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_StyledBackground, false);
    setAlignment(Qt::AlignCenter);

    m_overlayContainer = new OverlayContainer(this);
    m_overlayContainer->setFixedSize(60, 24);
    m_overlayContainer->hide();

    m_enlargeButton = new PreviewOverlayButton(QColor(250, 249, 246), QColor(255, 255, 255, 30), m_overlayContainer);
    m_deleteButton = new PreviewOverlayButton(QColor(250, 249, 246), QColor(255, 0, 0, 100), m_overlayContainer);
    auto *layout = new QHBoxLayout(m_overlayContainer);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(4);
    layout->addWidget(m_enlargeButton);
    layout->addWidget(m_deleteButton);

    m_overlayAnimation = new QVariantAnimation(this);
    m_overlayAnimation->setDuration(150);
    connect(m_overlayAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const int alpha = value.toInt();
        m_overlayContainer->setAlpha(alpha);
        m_enlargeButton->setAlpha(alpha);
        m_deleteButton->setAlpha(alpha);
        m_overlayContainer->setVisible(alpha != 0);
    });
    connect(m_deleteButton, &QAbstractButton::clicked, this, [this] { if (onDeleteClicked) onDeleteClicked(); });
    connect(m_enlargeButton, &QAbstractButton::clicked, this, [this] { if (onEnlargeClicked) onEnlargeClicked(); });
}

void StagingImageLabel::setOriginalPixmap(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    if (auto *view = qobject_cast<QAbstractItemView *>(parent())) {
        view->updateGeometry();
        view->update();
    }
}

const QPixmap &StagingImageLabel::originalPixmap() const
{
    return m_originalPixmap;
}

bool StagingImageLabel::hasHeightForWidth() const { return true; }

int StagingImageLabel::heightForWidth(int width) const
{
    if (m_originalPixmap.isNull() || m_originalPixmap.width() == 0)
        return width;
    return qMin(width * m_originalPixmap.height() / m_originalPixmap.width(), 400);
}

QSize StagingImageLabel::sizeHint() const
{
    const int imageWidth = width() > 0 ? width() : 200;
    return {imageWidth, heightForWidth(imageWidth)};
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void StagingImageLabel::enterEvent(QEnterEvent *event)
#else
void StagingImageLabel::enterEvent(QEvent *event)
#endif
{
    QLabel::enterEvent(event);
    m_overlayAnimation->stop();
    m_overlayAnimation->setStartValue(m_overlayContainer->alpha());
    m_overlayAnimation->setEndValue(255);
    m_overlayAnimation->start();
}

void StagingImageLabel::leaveEvent(QEvent *event)
{
    QLabel::leaveEvent(event);
    m_overlayAnimation->stop();
    m_overlayAnimation->setStartValue(m_overlayContainer->alpha());
    m_overlayAnimation->setEndValue(0);
    m_overlayAnimation->start();
}

void StagingImageLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    m_overlayContainer->move(width() - m_overlayContainer->width() - 8, 8);
}

void StagingImageLabel::paintEvent(QPaintEvent *event)
{
    if (m_originalPixmap.isNull()) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QPixmap scaled = m_originalPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    painter.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);
}
