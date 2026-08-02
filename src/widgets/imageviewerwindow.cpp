#include "imageviewerwindow.h"
#include "titlebarbutton.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

ImageViewerWindow::ImageViewerWindow(const QPixmap& pixmap, QWidget *parent)
    : QWidget(parent), 
      m_pixmap(pixmap), 
      m_scaleFactor(-1.0),
      m_isPanning(false)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose); // очистка памяти при закрытии окна
    setAttribute(Qt::WA_TranslucentBackground);

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME);
#endif

    m_buttonsContainer = new QWidget(this);
    m_buttonsContainer->setAttribute(Qt::WA_TranslucentBackground);

    QColor defaultCircleColor(250, 249, 246);

    m_minimizeBtn = new TitleBarButton(defaultCircleColor, QColor(255, 255, 255, 30), m_buttonsContainer);
    m_maximizeBtn = new TitleBarButton(defaultCircleColor, QColor(255, 255, 255, 30), m_buttonsContainer);
    m_closeBtn = new TitleBarButton(defaultCircleColor, QColor(255, 0, 0, 100), m_buttonsContainer);

    connect(m_minimizeBtn, &QAbstractButton::clicked, this, &ImageViewerWindow::onMinimizeClicked);
    connect(m_maximizeBtn, &QAbstractButton::clicked, this, &ImageViewerWindow::onMaximizeRestoreClicked);
    connect(m_closeBtn, &QAbstractButton::clicked, this, &ImageViewerWindow::onCloseClicked);

    QHBoxLayout *topLayout = new QHBoxLayout(m_buttonsContainer);
    topLayout->setContentsMargins(0, 5, 8, 0);
    topLayout->setSpacing(8);
    topLayout->addWidget(m_minimizeBtn);
    topLayout->addWidget(m_maximizeBtn);
    topLayout->addWidget(m_closeBtn);

    QHBoxLayout *mainTopLayout = new QHBoxLayout();
    mainTopLayout->setContentsMargins(0, 0, 0, 0);
    mainTopLayout->addStretch();
    mainTopLayout->addWidget(m_buttonsContainer);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(mainTopLayout);
    mainLayout->addStretch();

    m_buttonsContainer->hide();
}

void ImageViewerWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    if (!m_pixmap.isNull()) {
        if (!m_isManuallyZoomed || m_scaleFactor < 0) {
            QSizeF scaledSize = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
            m_scaleFactor = scaledSize.width() / m_pixmap.width();
            m_panOffset = QPointF(0, 0);
        } else {
            constrainPanOffset();
        }
    }
}

void ImageViewerWindow::constrainPanOffset()
{
    if (m_pixmap.isNull()) return;

    qreal currentScale = m_scaleFactor > 0 ? m_scaleFactor : 1.0;
    qreal scaledWidth = m_pixmap.width() * currentScale;
    qreal scaledHeight = m_pixmap.height() * currentScale;

    qreal marginX = qMin(100.0, scaledWidth / 2.0);
    qreal marginY = qMin(100.0, scaledHeight / 2.0);

    qreal maxOffsetX = (width() + scaledWidth) / 2.0 - marginX;
    qreal maxOffsetY = (height() + scaledHeight) / 2.0 - marginY;

    if (maxOffsetX < 0) maxOffsetX = 0;
    if (maxOffsetY < 0) maxOffsetY = 0;

    m_panOffset.setX(qBound(-maxOffsetX, m_panOffset.x(), maxOffsetX));
    m_panOffset.setY(qBound(-maxOffsetY, m_panOffset.y(), maxOffsetY));
}



void ImageViewerWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    
    // Нарисовать полупрозрачный фон
    painter.fillRect(rect(), QColor(10, 10, 10, 200));
    
    if (m_pixmap.isNull()) return;
    
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    qreal currentScale = m_scaleFactor > 0 ? m_scaleFactor : 1.0;
    qreal scaledWidth = m_pixmap.width() * currentScale;
    qreal scaledHeight = m_pixmap.height() * currentScale;

    
    qreal x = (width() - scaledWidth) / 2.0 + m_panOffset.x();
    qreal y = (height() - scaledHeight) / 2.0 + m_panOffset.y();
    
    painter.drawPixmap(QRectF(x, y, scaledWidth, scaledHeight), m_pixmap, m_pixmap.rect());
}

void ImageViewerWindow::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        qreal zoomFactor = 1.1;
        if (event->angleDelta().y() < 0) {
            zoomFactor = 1.0 / zoomFactor;
        }
        
        m_scaleFactor *= zoomFactor;
        
        QSizeF fitSize = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
        qreal fitScale = fitSize.width() / m_pixmap.width();

        if (m_scaleFactor <= fitScale) {
            m_scaleFactor = fitScale;
            m_isManuallyZoomed = false;
            m_panOffset = QPointF(0, 0);
        } else {
            m_isManuallyZoomed = true;
        }
        
        m_scaleFactor = qMax(0.1, qMin(m_scaleFactor, 20.0));
        constrainPanOffset();
        
        update();
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void ImageViewerWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qreal currentScale = m_scaleFactor > 0 ? m_scaleFactor : 1.0;
        qreal scaledWidth = m_pixmap.width() * currentScale;
        qreal scaledHeight = m_pixmap.height() * currentScale;
        
        qreal x = (width() - scaledWidth) / 2.0 + m_panOffset.x();
        qreal y = (height() - scaledHeight) / 2.0 + m_panOffset.y();
        
        QRectF imageRect(x, y, scaledWidth, scaledHeight);

        int titleBarHeight = 40;
        if (event->position().y() <= titleBarHeight && !isFullScreen()) {
            if (window()->windowHandle()) {
                window()->windowHandle()->startSystemMove();
            }
        } else if (imageRect.contains(event->position())) {
            m_isPanning = true;
            m_lastMousePos = event->position();
            setCursor(Qt::ClosedHandCursor);
        } else {
            m_isBackgroundPressed = true;
            m_backgroundClickPos = event->position();
        }
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void ImageViewerWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    int titleBarHeight = 40;
    if (event->button() == Qt::LeftButton && event->position().y() <= titleBarHeight && !isFullScreen()) {
        onMaximizeRestoreClicked();
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void ImageViewerWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPointF delta = event->position() - m_lastMousePos;
        m_panOffset += delta;
        m_lastMousePos = event->position();
        constrainPanOffset();
        update();
        event->accept();
    } else if (m_isBackgroundPressed) {
        if ((event->position() - m_backgroundClickPos).manhattanLength() > QApplication::startDragDistance()) {
            m_isBackgroundPressed = false;
            if (!isFullScreen()) {
                if (window()->windowHandle()) {
                    window()->windowHandle()->startSystemMove();
                }
            }
        }
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void ImageViewerWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isPanning) {
            m_isPanning = false;
            unsetCursor();
        }
        if (m_isBackgroundPressed) {
            m_isBackgroundPressed = false;
            close();
        }
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void ImageViewerWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ImageViewerWindow::onMinimizeClicked()
{
    showMinimized();
}

void ImageViewerWindow::onMaximizeRestoreClicked()
{
    bool nativeZoomed = false;
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    nativeZoomed = IsZoomed(hwnd);
#endif

    bool wasFullScreen = isFullScreen();

    if (wasFullScreen || isMaximized() || nativeZoomed) {
        showNormal();
#ifdef Q_OS_WIN
        if (nativeZoomed && !wasFullScreen) {
            ShowWindow(hwnd, SW_RESTORE);
        }
#endif
    } else {
        showFullScreen();
    }
}

void ImageViewerWindow::onCloseClicked()
{
    close();
}

bool ImageViewerWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCCALCSIZE) {
        if (msg->wParam == TRUE) {
            *result = 0;
            return true;
        }
        return false;
    }

    if (msg->message == WM_NCHITTEST) {
        int border_width = 8;
        
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        QPoint pos = mapFromGlobal(QPoint(x, y));

        bool left   = pos.x() < border_width;
        bool right  = pos.x() > width() - border_width;
        bool top    = pos.y() < border_width;
        bool bottom = pos.y() > height() - border_width;

        if (top && left)     { *result = HTTOPLEFT; return true;     }
        if (top && right)    { *result = HTTOPRIGHT; return true;    }
        if (bottom && left)  { *result = HTBOTTOMLEFT; return true;  }
        if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
        if (left)            { *result = HTLEFT; return true;        }
        if (right)           { *result = HTRIGHT; return true;       }
        if (top)             { *result = HTTOP; return true;         }
        if (bottom)          { *result = HTBOTTOM; return true;      }
    }
#endif
    return QWidget::nativeEvent(eventType, message, result);
}

void ImageViewerWindow::enterEvent(QEnterEvent *event)
{
    if (m_buttonsContainer) {
        m_buttonsContainer->show();
    }
    QWidget::enterEvent(event);
}

void ImageViewerWindow::leaveEvent(QEvent *event)
{
    if (m_buttonsContainer) {
        m_buttonsContainer->hide();
    }
    QWidget::leaveEvent(event);
}
