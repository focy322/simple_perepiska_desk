#ifndef IMAGECROPPERDIALOG_H
#define IMAGECROPPERDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>

class ImageCropperDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImageCropperDialog(const QPixmap &image, QWidget *parent = nullptr);
    QPixmap getCroppedImage() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPixmap m_image;
    double m_scale;
    QPointF m_offset;
    QPoint m_lastMousePos;
    bool m_isDragging;
    int m_cropSize;
};

#endif // IMAGECROPPERDIALOG_H
