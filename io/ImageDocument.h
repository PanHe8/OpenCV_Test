#pragma once
#include <QImage>
#include <QString>
#include <opencv2/core.hpp>

// Owns the decoded image and its Qt display copy; handles Unicode file paths.
class ImageDocument
{
public:
  bool load(const QString& path, QString& error);
  const cv::Mat& pixels() const { return pixels_; }
  const QImage& image() const { return image_; }
  const QString& path() const { return path_; }

private:
  cv::Mat pixels_;
  QImage image_;
  QString path_;
};
