`detrac_y5s_640.bin`: candidate boxes of 192 frames from 12 traffic cameras, in the order the
decoder writes them. Boxes of the same class in a frame have different confidence values.

- Frames: UA-DETRAC training set (CC BY-NC-SA 3.0). Only the detector outputs are stored here.
- Model: Ultralytics YOLOv5 v7.0 `yolov5s.onnx` (COCO, AGPL-3.0), 640x640.
- Format: "NMSC", uint32 frames, uint32 image size, then for every frame uint32 n and
  n x (cx, cy, w, h, conf, class_id) as float32.
