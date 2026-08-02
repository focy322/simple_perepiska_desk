#include "customtitlebar.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QWindow>
#include <QIcon>
#include <QSpacerItem>
#include <QPainter>
#include <QEnterEvent>

#include "titlebarbutton.h"

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent)
{
    // Толщина полоски
    setFixedHeight(23);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 5, 8, 0);
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

    connect(m_minimizeBtn, &QAbstractButton::clicked, this, &CustomTitleBar::onMinimizeClicked);
    connect(m_maximizeBtn, &QAbstractButton::clicked, this, &CustomTitleBar::onMaximizeRestoreClicked);
    connect(m_closeBtn, &QAbstractButton::clicked, this, &CustomTitleBar::onCloseClicked);

    // Поддержка QSS для кастомного QWidget
    setAttribute(Qt::WA_StyledBackground, true);

    // Стиль полоски
    setStyleSheet("CustomTitleBar { background-color: #0A0A0A; border-top-left-radius: 0px; border-top-right-radius: 0px; }");
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
