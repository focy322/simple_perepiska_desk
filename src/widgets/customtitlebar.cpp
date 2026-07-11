#include "customtitlebar.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QWindow>
#include <QIcon>
#include <QSpacerItem>

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(20);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(8);

    // Добавляем пружину слева, чтобы кнопки были справа
    layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    QIcon circleIcon(":/icons/circle.svg");

    m_minimizeBtn = new QPushButton(this);
    m_minimizeBtn->setIcon(circleIcon);
    m_minimizeBtn->setFixedSize(16, 16);
    m_minimizeBtn->setStyleSheet("QPushButton { border: none; background-color: transparent; } QPushButton:hover { background-color: rgba(255, 255, 255, 30); border-radius: 8px; }");
    m_minimizeBtn->setCursor(Qt::PointingHandCursor);

    m_maximizeBtn = new QPushButton(this);
    m_maximizeBtn->setIcon(circleIcon);
    m_maximizeBtn->setFixedSize(16, 16);
    m_maximizeBtn->setStyleSheet("QPushButton { border: none; background-color: transparent; } QPushButton:hover { background-color: rgba(255, 255, 255, 30); border-radius: 8px; }");
    m_maximizeBtn->setCursor(Qt::PointingHandCursor);

    m_closeBtn = new QPushButton(this);
    m_closeBtn->setIcon(circleIcon);
    m_closeBtn->setFixedSize(16, 16);
    m_closeBtn->setStyleSheet("QPushButton { border: none; background-color: transparent; } QPushButton:hover { background-color: rgba(255, 0, 0, 100); border-radius: 8px; }");
    m_closeBtn->setCursor(Qt::PointingHandCursor);

    layout->addWidget(m_minimizeBtn);
    layout->addWidget(m_maximizeBtn);
    layout->addWidget(m_closeBtn);

    connect(m_minimizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMinimizeClicked);
    connect(m_maximizeBtn, &QPushButton::clicked, this, &CustomTitleBar::onMaximizeRestoreClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &CustomTitleBar::onCloseClicked);

    // Стиль самой полоски (можно оставить прозрачной или задать цвет)
    setStyleSheet("background-color: transparent;");
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
