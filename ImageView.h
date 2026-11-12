#pragma once
#include "detection/CircleTypes.h"
#include <QGraphicsView>
#include <QImage>

// 图像显示控件：绘制检测结果，并处理缩放和平移。
class ImageView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit ImageView(QWidget* parent = nullptr);
  void setImage(const QImage& image);
  void setCircles(const std::vector<vision::Circle>& circles);
  void fitImage();
  void setCircleOffsetsVisible(bool visible);
  void clearAnnotations();

signals:
  void clearWindowRequested();

protected:
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  bool viewportEvent(QEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void drawForeground(QPainter* painter, const QRectF& rect) override;
  void paintEvent(QPaintEvent* event) override;

private:
  QGraphicsScene scene_;
  QImage image_;
  QList<QGraphicsItem*> overlays_;
  std::vector<vision::Circle> circles_;
  bool fieldCenterVisible_ = false;
  bool circleOffsetsVisible_ = false;
  bool pixelCoordinatesVisible_ = false;
  bool mouseInViewport_ = false;
  QPoint mousePosition_;
  bool autoFit_ = true;
};
