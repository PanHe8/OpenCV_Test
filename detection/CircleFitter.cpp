#include "detection/CircleFitter.h"
#include <cmath>

namespace vision {

bool CircleFitter::fit(const std::vector<cv::Point>& points,
                       Circle& circle) const
{
  if (points.size() < 6)
    return false;
  cv::Point2d mean(0, 0);
  for (const auto& p : points)
    mean += cv::Point2d(p);
  mean *= 1.0 / points.size();
  cv::Mat a(static_cast<int>(points.size()), 3, CV_64F);
  cv::Mat b(static_cast<int>(points.size()), 1, CV_64F);
  for (int i = 0; i < a.rows; ++i) {
    const double x = points[i].x - mean.x;
    const double y = points[i].y - mean.y;
    a.at<double>(i, 0) = 2 * x;
    a.at<double>(i, 1) = 2 * y;
    a.at<double>(i, 2) = 1;
    b.at<double>(i) = x * x + y * y;
  }
  cv::Mat singular;
  cv::SVD::compute(a, singular);
  if (singular.at<double>(2) < singular.at<double>(0) * 1e-8)
    return false;
  cv::Mat solution;
  if (!cv::solve(a, b, solution, cv::DECOMP_SVD))
    return false;
  cv::Point2d center(solution.at<double>(0) + mean.x,
                     solution.at<double>(1) + mean.y);
  const double radiusSquared = solution.at<double>(2) +
    solution.at<double>(0) * solution.at<double>(0) +
    solution.at<double>(1) * solution.at<double>(1);
  if (radiusSquared <= 0)
    return false;
  double radius = std::sqrt(radiusSquared);

  // Refine the algebraic initialization by minimizing geometric radial errors.
  for (int iteration = 0; iteration < 12; ++iteration) {
    cv::Mat jacobian(a.rows, 3, CV_64F);
    cv::Mat residual(a.rows, 1, CV_64F);
    for (int i = 0; i < a.rows; ++i) {
      const cv::Point2d delta = center - cv::Point2d(points[i]);
      const double distance = cv::norm(delta);
      if (distance < 1e-9)
        return false;
      jacobian.at<double>(i, 0) = delta.x / distance;
      jacobian.at<double>(i, 1) = delta.y / distance;
      jacobian.at<double>(i, 2) = -1;
      residual.at<double>(i) = radius - distance;
    }
    cv::Mat step;
    if (!cv::solve(jacobian, residual, step, cv::DECOMP_SVD))
      return false;
    center.x += step.at<double>(0);
    center.y += step.at<double>(1);
    radius += step.at<double>(2);
    if (cv::norm(step) < 1e-6)
      break;
  }
  if (!std::isfinite(center.x) || !std::isfinite(center.y) ||
      !std::isfinite(radius) || radius <= 0)
    return false;
  double squaredError = 0;
  for (const auto& p : points) {
    const double error = cv::norm(cv::Point2d(p) - center) - radius;
    squaredError += error * error;
  }
  circle.center = center;
  circle.radius = radius;
  circle.rmsError = std::sqrt(squaredError / points.size());
  return true;
}

}
