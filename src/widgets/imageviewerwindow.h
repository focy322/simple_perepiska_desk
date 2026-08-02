#ifndef IMAGEVIEWERWINDOW_H
#define IMAGEVIEWERWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QPointF>

class ImageViewerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ImageViewerWindow(const QPixmap& pixmap, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onMinimizeClicked();
    void onMaximizeRestoreClicked();
    void onCloseClicked();

private:
    void constrainPanOffset();

    QPixmap m_pixmap;
    qreal m_scaleFactor;
    QPointF m_panOffset;
    bool m_isManuallyZoomed = false;
    
    bool m_isPanning;
    QPointF m_lastMousePos;
    
    bool m_isBackgroundPressed = false;
    QPointF m_backgroundClickPos;

    QWidget *m_buttonsContainer;
    class TitleBarButton *m_minimizeBtn;
    class TitleBarButton *m_maximizeBtn;
    class TitleBarButton *m_closeBtn;
};

#endif // IMAGEVIEWERWINDOW_H
