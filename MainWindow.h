#pragma once

#include "detection/CircleDetector.h"
#include "io/ImageDocument.h"
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

// 协调界面操作、图片读取和圆检测，算法与绘制由独立类完成。
class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;
  bool loadImage(const QString& path);
  bool runDetection();
  bool saveReport(const QString& path) const;

private slots:
  void openImage();
  void detectCircles();
  void showCircleOffsets();
  void clearWindow();
  void exportResults();
  void clearLog();
  void setAutoThreshold(bool checked);

private:
  void connectSignals();
  void invalidateResults();
  void appendLog(const QString& text);
  vision::DetectionParameters parameters() const;
  Ui::MainWindow* ui_;
  ImageDocument document_;
  vision::CircleDetector detector_;
  vision::DetectionResult result_;
  bool resultValid_ = false;
};
