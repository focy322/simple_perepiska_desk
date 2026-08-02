#include "titlebarbutton.h"
#include <QPainter>
#include <QEnterEvent>

TitleBarButton::TitleBarButton(const QColor& defaultColor, const QColor& hoverBgColor, QWidget* parent)
    : QAbstractButton(parent), m_defaultColor(defaultColor), m_hoverBgColor(hoverBgColor), m_isHovered(false) {
    setFixedSize(18, 18);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_StyledBackground, false);
}

void TitleBarButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_isHovered) {
        painter.setBrush(m_hoverBgColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, width(), height());
    }

    painter.setBrush(m_defaultColor);
    painter.setPen(Qt::NoPen);
    // Рисуем внутренний круг
    painter.drawEllipse(4, 4, width() - 8, height() - 8);
}

void TitleBarButton::enterEvent(QEnterEvent* event) {
    m_isHovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void TitleBarButton::leaveEvent(QEvent* event) {
    m_isHovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}
