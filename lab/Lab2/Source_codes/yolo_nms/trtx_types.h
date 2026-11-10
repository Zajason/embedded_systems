#pragma once
// From tensorrtx (https://github.com/wang-xinyu/tensorrtx, MIT licence,
// Copyright (c) 2019-2020 Wang Xinyu): yolov5/src/types.h and the two constants of
// yolov5/src/config.h that nms() uses. Only change: the constants can be set with -D.
#ifndef TRTX_MAX_OUT
#define TRTX_MAX_OUT 25200  // upstream default kMaxNumOutputBbox = 1000; 25200 = whole 640x640 head
#endif
constexpr static int kMaxNumOutputBbox = TRTX_MAX_OUT;
constexpr static int kNumAnchor = 3;

struct alignas(float) Detection {
  float bbox[4];  // center_x center_y w h
  float conf;  // bbox_conf * cls_conf
  float class_id;
#ifndef TRTX_NO_MASK   // -DTRTX_NO_MASK: 24-byte detection (element-size experiment)
  float mask[32];
#endif
};

void nms(std::vector<Detection>& res, float* output, float conf_thresh, float nms_thresh = 0.5);
