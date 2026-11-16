// ORIGINAL: iou(), cmp() and nms() copied verbatim from tensorrtx
//   https://github.com/wang-xinyu/tensorrtx/blob/master/yolov5/src/postprocess.cpp
// The OpenCV drawing / mask functions of that file are not needed and were left out.
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>
#include <ddtr_library>
#include "trtx_types.h"
#include "allocators.hpp"

#ifndef sel_dt_dets
#define sel_dt_dets vector
#endif
#ifndef sel_alloc_dets
#define sel_alloc_dets ddtr::malloc_allocator
#endif
using Dets = ddtr::sel_dt_dets<Detection, sel_alloc_dets>;
// Ζητούμενο 4: one flag per box (1 = rejected), through the same allocator as Dets.
using Flags = ddtr::vector<unsigned char, sel_alloc_dets>;

static float iou(float lbox[4], float rbox[4]) {
  float interBox[] = {
    (std::max)(lbox[0] - lbox[2] / 2.f , rbox[0] - rbox[2] / 2.f), //left
    (std::min)(lbox[0] + lbox[2] / 2.f , rbox[0] + rbox[2] / 2.f), //right
    (std::max)(lbox[1] - lbox[3] / 2.f , rbox[1] - rbox[3] / 2.f), //top
    (std::min)(lbox[1] + lbox[3] / 2.f , rbox[1] + rbox[3] / 2.f), //bottom
  };

  if (interBox[2] > interBox[3] || interBox[0] > interBox[1])
    return 0.0f;

  float interBoxS = (interBox[1] - interBox[0])*(interBox[3] - interBox[2]);
  return interBoxS / (lbox[2] * lbox[3] + rbox[2] * rbox[3] - interBoxS);
}

// Dets::sort() orders by operator<, ascending. "Smaller" = higher confidence, so the
// result is in descending confidence, like cmp() in the original.
static bool operator<(const Detection& a, const Detection& b) {
  return a.conf > b.conf;
}

void nms(std::vector<Detection>& res, float* output, float conf_thresh, float nms_thresh) {
  int det_size = sizeof(Detection) / sizeof(float);
  std::map<float, Dets> m;
  for (int i = 0; i < output[0] && i < kMaxNumOutputBbox; i++) {
    if (output[1 + det_size * i + 4] <= conf_thresh) continue;
    Detection det;
    memcpy(&det, &output[1 + det_size * i], det_size * sizeof(float));
    if (m.count(det.class_id) == 0) m.emplace(det.class_id, Dets());
    m[det.class_id].push_back(det);
  }
  for (auto it = m.begin(); it != m.end(); it++) {
    auto& dets = it->second;
    dets.sort();
#ifdef NMS_FLAG
    // Ζητούμενο 4: the rejected boxes are not erased, only flagged; dets never changes.
    // removed[i] belongs to the i-th box in sorted order, so i and j count along with the
    // iterators.
    const std::size_t count = dets.size();
    Flags removed;
    removed.reserve(count);
    removed.resize(count, 0);
    std::size_t i = 0;
    for (auto it_m = dets.begin(); it_m != dets.end(); ++it_m, ++i) {
      if (removed[i]) continue;
      auto& item = *it_m;
      res.push_back(item);
      auto it_n = it_m;
      ++it_n;
      for (std::size_t j = i + 1; it_n != dets.end(); ++it_n, ++j)
        if (!removed[j] && iou(item.bbox, it_n->bbox) > nms_thresh)
          removed[j] = 1;
    }
#else
    // Ζητούμενο 2: erase the rejected boxes.
    // Iterators instead of dets[m] / dets.begin() + n: on a list those walk from the head.
    for (auto it_m = dets.begin(); it_m != dets.end(); ++it_m) {
      auto& item = *it_m;
      res.push_back(item);
      auto it_n = it_m;
      ++it_n;
      while (it_n != dets.end()) {
        if (iou(item.bbox, it_n->bbox) > nms_thresh)
          it_n = dets.erase(it_n);  // returns the next element
        else
          ++it_n;
      }
    }
#endif
  }
}
