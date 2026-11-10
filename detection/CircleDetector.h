#pragma once

#include "detection/CircleTypes.h"

namespace vision {

// Converts/segments the image, filters contours and delegates circle fitting.
// Input is never modified. Invalid input/parameters throw
// std::invalid_argument.
class CircleDetector
{
public:
  DetectionResult detect(
    const cv::Mat& image,
    const DetectionParameters& parameters = DetectionParameters()) const;
};

}
