#include "detection/CircleDetector.h"
#include "detection/CircleFitter.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vision {

DetectionResult CircleDetector::detect(const cv::Mat& image,
                                       const DetectionParameters& p) const
{
  if (image.empty() || image.depth() != CV_8U ||
      (image.channels() != 1 && image.channels() != 3 &&
       image.channels() != 4))
    throw std::invalid_argument(
      "Expected a nonempty 8-bit gray, BGR or BGRA image.");
  const double maxRadius =
    p.maxRadius == 0 ? std::min(image.cols, image.rows) * 0.2 : p.maxRadius;
  if (!std::isfinite(p.minRadius) || !std::isfinite(maxRadius) ||
      p.minRadius <= 0 || maxRadius < p.minRadius ||
      !std::isfinite(p.minCircularity) || p.minCircularity <= 0 ||
      p.minCircularity > 1 || !std::isfinite(p.maxRelativeError) ||
      p.maxRelativeError <= 0 || p.maxRelativeError > 1 || p.threshold < -1 ||
      p.threshold > 255)
    throw std::invalid_argument(
      "Invalid radius, circularity, fit error or threshold parameters.");

  const int64 start = cv::getTickCount();
  cv::Mat gray, blurred, mask;
  if (image.channels() == 3)
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
  else if (image.channels() == 4)
    cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
  else
    gray = image;
  cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 1.0);
  int mode = p.foreground == Foreground::Dark ? cv::THRESH_BINARY_INV
                                              : cv::THRESH_BINARY;
  if (p.threshold < 0)
    mode |= cv::THRESH_OTSU;
  DetectionResult result;
  result.threshold =
    cv::threshold(blurred, mask, std::max(0, p.threshold), 255, mode);
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
  CircleFitter fitter;
  for (const auto& contour : contours) {
    if (contour.size() < 20)
      continue;
    const cv::Rect bounds = cv::boundingRect(contour);
    if (bounds.x <= 0 || bounds.y <= 0 || bounds.br().x >= image.cols ||
        bounds.br().y >= image.rows)
      continue; // Partial circles are outside the scope.
    const double area = std::abs(cv::contourArea(contour));
    const double perimeter = cv::arcLength(contour, true);
    if (perimeter <= 0 || area < CV_PI * p.minRadius * p.minRadius * 0.7)
      continue;
    const double circularity = 4 * CV_PI * area / (perimeter * perimeter);
    if (circularity < p.minCircularity)
      continue;
    Circle circle;
    if (!fitter.fit(contour, circle))
      continue;
    if (circle.radius < p.minRadius || circle.radius > maxRadius ||
        circle.rmsError / circle.radius > p.maxRelativeError)
      continue;
    circle.circularity = circularity;
    result.circles.push_back(circle);
  }
  // Collapse near-identical inner/outer boundaries of thin circular rings.
  std::sort(
    result.circles.begin(), result.circles.end(),
    [](const Circle& a, const Circle& b) { return a.rmsError < b.rmsError; });
  std::vector<Circle> unique;
  for (const auto& circle : result.circles) {
    bool duplicate = false;
    for (const auto& previous : unique) {
      const double tolerance = std::max(3.0, 0.03 * circle.radius);
      if (cv::norm(previous.center - circle.center) < tolerance &&
          std::abs(previous.radius - circle.radius) < tolerance)
        duplicate = true;
    }
    if (!duplicate)
      unique.push_back(circle);
  }
  std::sort(unique.begin(), unique.end(),
            [](const Circle& a, const Circle& b) {
              if (a.center.y != b.center.y)
                return a.center.y < b.center.y;
              return a.center.x < b.center.x;
            });
  result.circles.swap(unique);
  result.elapsedMs =
    (cv::getTickCount() - start) * 1000.0 / cv::getTickFrequency();
  return result;
}

}
