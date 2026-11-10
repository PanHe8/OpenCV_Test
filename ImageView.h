#pragma once
#include "detection/CircleTypes.h"
#include <QGraphicsView>
#include <QImage>

// 图像显示控件：绘制检测结果，并处理缩放和平移。
class ImageView : public QGraphicsView
{
public:
  explicit ImageView(QWidget* parent = nullptr);
  void setImage(const QImage& image);
  void setCircles(const std::vector<vision::Circle>& circles);
  void fitImage();

protected:
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  QGraphicsScene scene_;
  QImage image_;
  QList<QGraphicsItem*> overlays_;
  bool autoFit_ = true;
};
