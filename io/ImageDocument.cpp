#include "io/ImageDocument.h"
#include <QFile>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

bool ImageDocument::load(const QString& path, QString& error)
{
  // Clear first so failed loads cannot leave a previous image active.
  pixels_.release();
  image_ = QImage();
  path_.clear();
  error.clear();
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    error = QStringLiteral("无法打开图片：%1").arg(file.errorString());
    return false;
  }
  const QByteArray bytes = file.readAll();
  if (bytes.startsWith("%TSD-Header-###%")) {
    error =
      QStringLiteral("当前程序读到的是文件加密头，无法解码。请使用本机加密软件"
                     "允许本程序读取的图片，或选择正常导出的图片副本。");
    return false;
  }
  try {
    const std::vector<uchar> encoded(bytes.begin(), bytes.end());
    if (!encoded.empty())
      pixels_ = cv::imdecode(encoded, cv::IMREAD_COLOR);
    if (pixels_.empty()) {
      error =
        QStringLiteral("图片解码失败，请选择有效的 BMP、PNG 或 JPEG 图片。");
      return false;
    }
    cv::Mat rgb;
    cv::cvtColor(pixels_, rgb, cv::COLOR_BGR2RGB);
    image_ = QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                    QImage::Format_RGB888)
               .copy();
    path_ = path;
    return true;
  } catch (const cv::Exception& exception) {
    pixels_.release();
    error = QString::fromLocal8Bit(exception.what());
    return false;
  }
}
