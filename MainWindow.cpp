#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHeaderView>
#include <QSaveFile>
#include <QTextStream>
#include <exception>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , ui_(new Ui::MainWindow)
{
  ui_->setupUi(this);
  connectSignals();
  ui_->splitter->setSizes({ 720, 640 });
  ui_->resultEdit->setFont(
    QFontDatabase::systemFont(QFontDatabase::FixedFont));
  ui_->resultsTable->horizontalHeader()->setSectionResizeMode(
    QHeaderView::ResizeToContents);
  ui_->resultsTable->horizontalHeader()->setStretchLastSection(true);
  ui_->resultsTable->verticalHeader()->setVisible(false);
  ui_->resultsTable->verticalHeader()->setDefaultSectionSize(28);
  appendLog(QStringLiteral(
    "识别记录会保留在这里，可滚动查阅，按 Ctrl+A、Ctrl+C 复制。"));
}

MainWindow::~MainWindow()
{
  delete ui_;
}

void MainWindow::connectSignals()
{
  // 所有界面连接集中写在源码中，不使用 on_ 命名自动连接。
  connect(ui_->openButton, &QPushButton::clicked, this, &MainWindow::openImage);
  connect(ui_->detectButton, &QPushButton::clicked, this, &MainWindow::detectCircles);
  connect(ui_->fitButton, &QPushButton::clicked, this, &MainWindow::showCircleOffsets);
  connect(ui_->imageView, &ImageView::clearWindowRequested, this, &MainWindow::clearWindow);
  connect(ui_->exportButton, &QPushButton::clicked, this, &MainWindow::exportResults);
  connect(ui_->clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
  connect(ui_->minRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::invalidateResults);
  connect(ui_->maxRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::invalidateResults);
  connect(ui_->circularitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::invalidateResults);
  connect(ui_->fitErrorSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::invalidateResults);
  connect(ui_->thresholdSpin, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &MainWindow::invalidateResults);
  connect(ui_->foregroundCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::invalidateResults);
  connect(ui_->autoThresholdCheck, &QCheckBox::toggled,
          this, &MainWindow::setAutoThreshold);
}

void MainWindow::openImage()
{
  const QString path = QFileDialog::getOpenFileName(
    this, QStringLiteral("选择图片"), document_.path(),
    QStringLiteral(
      "图片 (*.bmp *.png *.jpg *.jpeg *.tif *.tiff);;所有文件 (*)"));
  if (!path.isEmpty())
    loadImage(path);
}

void MainWindow::detectCircles()
{
  runDetection();
}
void MainWindow::showCircleOffsets()
{
  if (!resultValid_ || result_.circles.empty()) {
    statusBar()->showMessage(QStringLiteral("请先识别，得到有效的圆。"));
    return;
  }
  ui_->imageView->setCircleOffsetsVisible(true);
  statusBar()->showMessage(QStringLiteral("已显示圆心与视场中心的坐标差值"));
}

void MainWindow::clearWindow()
{
  invalidateResults();
  ui_->imageView->clearAnnotations();
  ui_->summaryLabel->setText(QStringLiteral("已清空窗口，仅显示原图，请点击识别。"));
  statusBar()->showMessage(QStringLiteral("已清空窗口，仅保留原图"));
}
void MainWindow::clearLog()
{
  ui_->resultEdit->clear();
}

void MainWindow::exportResults()
{
  if (!resultValid_) {
    statusBar()->showMessage(QStringLiteral("请先执行识别，得到有效结果。"));
    return;
  }
  const QString path = QFileDialog::getSaveFileName(
    this, QStringLiteral("导出结果"), QStringLiteral("circles.csv"),
    QStringLiteral("CSV (*.csv)"));
  if (path.isEmpty())
    return;
  const QString message = saveReport(path)
    ? QStringLiteral("结果已导出：%1").arg(path)
    : QStringLiteral("导出失败，请检查目标路径。");
  statusBar()->showMessage(message);
  appendLog(message);
}

void MainWindow::setAutoThreshold(bool checked)
{
  ui_->thresholdSpin->setEnabled(!checked);
  invalidateResults();
}

void MainWindow::appendLog(const QString& text)
{
  ui_->resultEdit->appendPlainText(text);
}

void MainWindow::invalidateResults()
{
  result_ = vision::DetectionResult();
  resultValid_ = false;
  ui_->fitButton->setEnabled(false);
  ui_->resultsTable->setRowCount(0);
  ui_->imageView->setCircles({});
  ui_->summaryLabel->setText(
    QStringLiteral("尚未识别 / 参数已变化，请重新识别"));
  // 不清空编辑框中的历史记录，便于比较换图和调参前后的结果。
}

vision::DetectionParameters MainWindow::parameters() const
{
  vision::DetectionParameters p;
  p.minRadius = ui_->minRadiusSpin->value();
  p.maxRadius = ui_->maxRadiusSpin->value();
  p.minCircularity = ui_->circularitySpin->value();
  p.maxRelativeError = ui_->fitErrorSpin->value() / 100.0;
  p.threshold =
    ui_->autoThresholdCheck->isChecked() ? -1 : ui_->thresholdSpin->value();
  p.foreground = ui_->foregroundCombo->currentIndex() == 0
    ? vision::Foreground::Dark
    : vision::Foreground::Light;
  return p;
}

bool MainWindow::loadImage(const QString& path)
{
  invalidateResults();
  QString error;
  const bool loaded = document_.load(path, error);
  ui_->detectButton->setEnabled(loaded);
  ui_->imageView->setImage(document_.image());
  ui_->imageInfoLabel->setText(loaded ? QStringLiteral("%1\n%2 × %3 像素")
                                          .arg(QFileInfo(path).fileName())
                                          .arg(document_.pixels().cols)
                                          .arg(document_.pixels().rows)
                                      : error);
  statusBar()->showMessage(loaded ? QStringLiteral("图片已加载") : error);
  appendLog(
    QStringLiteral("\n[%1] %2\n图片：%3")
      .arg(QDateTime::currentDateTime().toString(
        QStringLiteral("yyyy-MM-dd HH:mm:ss")))
      .arg(loaded ? QStringLiteral("加载成功") : QStringLiteral("加载失败"))
      .arg(path));
  if (!loaded)
    appendLog(error);
  return loaded;
}

bool MainWindow::runDetection()
{
  invalidateResults();
  if (document_.pixels().empty()) {
    appendLog(QStringLiteral("无法识别：请先加载有效图片。"));
    return false;
  }
  const auto p = parameters();
  appendLog(QStringLiteral("\n[%1] 开始识别：%2")
              .arg(QDateTime::currentDateTime().toString(
                QStringLiteral("yyyy-MM-dd HH:mm:ss")))
              .arg(document_.path()));
  appendLog(QStringLiteral("尺寸：%1 × %2 像素；半径范围：%3 ～ "
                           "%4；最小圆度：%5；最大相对拟合误差：%6%")
              .arg(document_.pixels().cols)
              .arg(document_.pixels().rows)
              .arg(p.minRadius, 0, 'f', 1)
              .arg(p.maxRadius == 0 ? QStringLiteral("自动（短边的 20%）")
                                    : QString::number(p.maxRadius, 'f', 1))
              .arg(p.minCircularity, 0, 'f', 2)
              .arg(p.maxRelativeError * 100, 0, 'f', 1));
  appendLog(QStringLiteral("目标：%1；阈值模式：%2")
              .arg(ui_->foregroundCombo->currentText())
              .arg(p.threshold < 0 ? QStringLiteral("自动阈值")
                                   : QString::number(p.threshold)));
  QApplication::setOverrideCursor(Qt::WaitCursor);
  try {
    result_ = detector_.detect(document_.pixels(), p);
  } catch (const std::exception& error) {
    QApplication::restoreOverrideCursor();
    const QString message =
      QStringLiteral("识别失败：%1").arg(QString::fromLocal8Bit(error.what()));
    ui_->summaryLabel->setText(message);
    appendLog(message);
    return false;
  }
  QApplication::restoreOverrideCursor();
  resultValid_ = true;
  ui_->imageView->setCircles(result_.circles);
  ui_->fitButton->setEnabled(!result_.circles.empty());
  ui_->resultsTable->setRowCount(static_cast<int>(result_.circles.size()));
  for (int i = 0; i < ui_->resultsTable->rowCount(); ++i) {
    const auto& c = result_.circles[i];
    const QStringList values = { QString::number(i + 1),
                                 QString::number(c.center.x, 'f', 3),
                                 QString::number(c.center.y, 'f', 3),
                                 QString::number(c.radius, 'f', 3),
                                 QString::number(c.radius * 2, 'f', 3),
                                 QString::number(c.rmsError, 'f', 3),
                                 QString::number(c.circularity, 'f', 3) };
    for (int j = 0; j < values.size(); ++j)
      ui_->resultsTable->setItem(i, j, new QTableWidgetItem(values[j]));
    appendLog(
      QStringLiteral(
        "圆 #%1：圆心 X=%2，Y=%3；半径=%4；直径=%5；拟合误差=%6；圆度=%7")
        .arg(values[0])
        .arg(values[1])
        .arg(values[2])
        .arg(values[3])
        .arg(values[4])
        .arg(values[5])
        .arg(values[6]));
  }
  const QString summary =
    QStringLiteral("检测到 %1 个圆 | 阈值 %2 | 耗时 %3 毫秒")
      .arg(result_.circles.size())
      .arg(result_.threshold, 0, 'f', 0)
      .arg(result_.elapsedMs, 0, 'f', 1);
  ui_->summaryLabel->setText(summary);
  appendLog(
    QStringLiteral(
      "识别完成：%1\n坐标原点：原图左上角；X 向右、Y 向下；长度单位：像素。\n")
      .arg(summary));
  if (result_.circles.empty())
    appendLog(
      QStringLiteral("未发现符合条件的圆，请检查目标极性、半径及筛选参数。"));
  ui_->resultsTabs->setCurrentIndex(0);
  statusBar()->showMessage(QStringLiteral("识别完成"));
  return true;
}

bool MainWindow::saveReport(const QString& path) const
{
  if (!resultValid_)
    return false;
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  stream.setRealNumberPrecision(6);
  stream << QStringLiteral("编号,圆心X（像素）,圆心Y（像素）,半径（像素）,"
                           "直径（像素）,拟合误差（像素）,圆度\n");
  int id = 0;
  for (const auto& c : result_.circles)
    stream << ++id << ',' << c.center.x << ',' << c.center.y << ',' << c.radius
           << ',' << c.radius * 2 << ',' << c.rmsError << ',' << c.circularity
           << '\n';
  stream.flush();
  return stream.status() == QTextStream::Ok && file.commit();
}
