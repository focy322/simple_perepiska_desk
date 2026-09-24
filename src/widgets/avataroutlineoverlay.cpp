#include "avataroutlineoverlay.h"

#include <QPainter>

AvatarOutlineOverlay::AvatarOutlineOverlay(QWidget *parent) : QLabel(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
}

void AvatarOutlineOverlay::setAlpha(int alpha)
{
    if (m_alpha != alpha) {
        m_alpha = alpha;
        update();
    }
}

void AvatarOutlineOverlay::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (m_alpha <= 0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255, m_alpha), 2));
    painter.setBrush(Qt::NoBrush);
    const int centerX = parentWidget() ? parentWidget()->width() / 2 : width() / 2;
    const int centerY = parentWidget() ? parentWidget()->height() / 2 : height() / 2;
    painter.drawEllipse(QRect(centerX - 23, centerY - 23, 46, 46));
}
