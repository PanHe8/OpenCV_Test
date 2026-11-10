#pragma once

#include <opencv2/core.hpp>
#include <vector>

namespace vision {

enum class Foreground
{
  Dark,
  Light
};

struct DetectionParameters
{
  double minRadius = 15.0;
  double maxRadius = 0.0; // Zero: 20% of the shorter image side.
  double minCircularity = 0.75;
  double maxRelativeError = 0.07;
  int threshold = -1; // Negative: Otsu automatic threshold.
  Foreground foreground = Foreground::Dark;
};

struct Circle
{
  cv::Point2d center;
  double radius = 0.0;
  double rmsError = 0.0; // RMS radial distance from the fitted circle, pixels.
  double circularity = 0.0;
};

struct DetectionResult
{
  std::vector<Circle> circles;
  double threshold = 0.0;
  double elapsedMs = 0.0;
};

} // namespace vision
