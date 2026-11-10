#pragma once

#include "detection/CircleTypes.h"

namespace vision {

// Fits contour points; has no image segmentation or Qt responsibilities.
class CircleFitter
{
public:
  bool fit(const std::vector<cv::Point>& points, Circle& circle) const;
};

}
