#include "customtitlebar.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QWindow>
#include <QIcon>
#include <QSpacerItem>
#include <QPainter>
#include <QEnterEvent>

class TitleBarButton : public QPushButton {
public:
    TitleBarButton(const QColor& defaultColor, const QColor& hoverBgColor, QWidget* parent = nullptr)
        : QPushButton(parent), m_defaultColor(defaultColor), m_hoverBgColor(hoverBgColor), m_isHovered(false) {
        setFixedSize(16, 16);
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
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

    void enterEvent(QEnterEvent* event) override {
        m_isHovered = true;
        update();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        m_isHovered = false;
        update();
        QPushButton::leaveEvent(event);
    }

private:
    QColor m_defaultColor;
    QColor m_hoverBgColor;
    bool m_isHovered;
};

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(20);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(8);

    // Добавляем пружину слева, чтобы кнопки были справа
    layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    QColor defaultCircleColor(250, 249, 246);

    m_minimizeBtn = new TitleBarButton(defaultCircleColor, QColor(255, 255, 255, 30), this);
    m_maximizeBtn = new TitleBarButton(defaultCircleColor, QColor(255, 255, 255, 30), this);
    m_closeBtn = new TitleBarButton(defaultCircleColor, QColor(255, 0, 0, 100), this);

    layout->addWidget(m_minimizeBtn);
    layout->addWidget(m_maximizeBtn);
    layout->addWidget(m_closeBtn);

    connect(m_minimizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMinimizeClicked);
    connect(m_maximizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMaximizeRestoreClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &CustomTitleBar::onCloseClicked);

    // Стиль самой полоски
    setStyleSheet("background-color: #141414; border-top-left-radius: 0px; border-top-right-radius: 0px;");
}

void CustomTitleBar::onMinimizeClicked()
{
    window()->showMinimized();
}

void CustomTitleBar::onMaximizeRestoreClicked()
{
    if (window()->isMaximized()) {
        window()->showNormal();
    } else {
        window()->showMaximized();
    }
}

void CustomTitleBar::onCloseClicked()
{
    window()->close();
}
