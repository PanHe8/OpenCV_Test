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
  // setupUi 内部通过 connectSlotsByName 自动连接。
  void on_openButton_clicked();
  void on_detectButton_clicked();
  void on_fitButton_clicked();
  void on_imageView_clearWindowRequested();
  void on_exportButton_clicked();
  void on_clearLogButton_clicked();
  void on_minRadiusSpin_valueChanged(double value);
  void on_maxRadiusSpin_valueChanged(double value);
  void on_circularitySpin_valueChanged(double value);
  void on_fitErrorSpin_valueChanged(double value);
  void on_thresholdSpin_valueChanged(int value);
  void on_foregroundCombo_currentIndexChanged(int index);
  void on_autoThresholdCheck_toggled(bool checked);

private:
  void invalidateResults();
  void appendLog(const QString& text);
  vision::DetectionParameters parameters() const;
  Ui::MainWindow* ui_;
  ImageDocument document_;
  vision::CircleDetector detector_;
  vision::DetectionResult result_;
  bool resultValid_ = false;
};
