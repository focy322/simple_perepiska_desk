#include "imagecropperdialog.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPainterPath>

ImageCropperDialog::ImageCropperDialog(const QPixmap &image, QWidget *parent)
    : QDialog(parent), m_image(image), m_scale(1.0), m_isDragging(false), m_cropSize(200)
{
    setFixedSize(400, 500);
    setStyleSheet("background-color: #1E2328; color: white;");

    // Начальный масштаб для соответствия размеру обрезки
    double scaleX = (double)m_cropSize / m_image.width();
    double scaleY = (double)m_cropSize / m_image.height();
    m_scale = qMax(scaleX, scaleY);
    m_offset = QPointF(width() / 2.0 - m_image.width() * m_scale / 2.0, 200 - m_image.height() * m_scale / 2.0);

    QPushButton *btnOk = new QPushButton("Сохранить", this);
    btnOk->setStyleSheet("background-color: #2F6BFF; border: none; border-radius: 5px; padding: 10px; color: white; font-weight: bold;");
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);

    QPushButton *btnCancel = new QPushButton("Отмена", this);
    btnCancel->setStyleSheet("background-color: #3A424B; border: none; border-radius: 5px; padding: 10px; color: white; font-weight: bold;");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(btnCancel);
    hLayout->addWidget(btnOk);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addStretch();
    vLayout->addLayout(hLayout);
}

void ImageCropperDialog::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Рисуем изображение
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);
    painter.drawPixmap(0, 0, m_image);
    painter.resetTransform();

    // Рисуем оверлей
    QPainterPath path;
    path.addRect(rect());
    path.addEllipse(QRectF(width() / 2.0 - m_cropSize / 2.0, 200 - m_cropSize / 2.0, m_cropSize, m_cropSize));
    
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.setPen(Qt::NoPen);
    painter.drawPath(path);

    // Рисуем круговую рамку
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawEllipse(QRectF(width() / 2.0 - m_cropSize / 2.0, 200 - m_cropSize / 2.0, m_cropSize, m_cropSize));
}

void ImageCropperDialog::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

void ImageCropperDialog::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update();
    }
}

void ImageCropperDialog::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

void ImageCropperDialog::wheelEvent(QWheelEvent *event) {
    double scaleFactor = 1.1;
    if (event->angleDelta().y() < 0) {
        scaleFactor = 1.0 / scaleFactor;
    }

    QPointF center = QPointF(width() / 2.0, 200);
    QPointF beforeScale = (center - m_offset) / m_scale;
    
    m_scale *= scaleFactor;
    
    // Убеждаемся, что он не станет слишком маленьким
    double minScaleX = (double)m_cropSize / m_image.width();
    double minScaleY = (double)m_cropSize / m_image.height();
    m_scale = qMax(m_scale, qMax(minScaleX, minScaleY));

    QPointF afterScale = beforeScale * m_scale;
    m_offset = center - afterScale;
    
    update();
}

QPixmap ImageCropperDialog::getCroppedImage() const {
    QPixmap target(m_cropSize, m_cropSize);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Создаем круговую маску
    QPainterPath path;
    path.addEllipse(0, 0, m_cropSize, m_cropSize);
    painter.setClipPath(path);
    
    // Рисуем часть изображения
    QPointF imgOffset = m_offset - QPointF(width() / 2.0 - m_cropSize / 2.0, 200 - m_cropSize / 2.0);
    painter.translate(imgOffset);
    painter.scale(m_scale, m_scale);
    painter.drawPixmap(0, 0, m_image);
    
    return target;
}
