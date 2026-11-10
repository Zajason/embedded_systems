#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <vector>
#include "trtx_types.h"

#ifdef ALLOC_STATS
#include "allocators.hpp"
#endif

#if defined(RUN_ORIG) || defined(VALIDATE)
void nms_orig(std::vector<Detection>& res, float* output, float conf_thresh, float nms_thresh);  // upstream/nms_orig.cpp
#endif

const float NMS_THRESHOLD = 0.45f;

// File: "NMSC", frames, image size, and for every frame n and n boxes of 6 floats.
// nms() expects every frame as [n, Detection, Detection, ...].
std::vector<float*> load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf("cannot open %s\n", path); exit(1); }
    uint32_t header[3];
    fread(header, 4, 3, f);
    const int det_size = sizeof(Detection) / sizeof(float);
    std::vector<float*> frames;
    for (uint32_t i = 0; i < header[1]; i++) {
        uint32_t n;
        fread(&n, 4, 1, f);
        // mmap, so that the input is not part of the heap
        float* frame = (float*)mmap(nullptr, (1 + n * det_size) * sizeof(float), PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        frame[0] = n;
        for (uint32_t j = 0; j < n; j++)
            fread(&frame[1 + j * det_size], sizeof(float), 6, f);
        frames.push_back(frame);
    }
    fclose(f);
    return frames;
}

#ifdef VALIDATE
// Only the fields of the original Detection, so that a field you add to it does not count
static bool same(const Detection& a, const Detection& b) {
    return !memcmp(a.bbox, b.bbox, sizeof(a.bbox)) && a.conf == b.conf && a.class_id == b.class_id;
}

static bool same(const std::vector<Detection>& a, const std::vector<Detection>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++)
        if (!same(a[i], b[i])) return false;
    return true;
}
#endif

int main(int argc, char** argv) {
    const char* input = "";
    float conf = 0.05f;
    int runs = 1;
    for (int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "input=", 6)) input = argv[i] + 6;
        if (!strncmp(argv[i], "conf=", 5)) conf = atof(argv[i] + 5);
        if (!strncmp(argv[i], "runs=", 5)) runs = atoi(argv[i] + 5);
    }

    std::vector<float*> frames = load(input);
    std::vector<Detection> res;
    res.reserve(4096);  // so that it does not grow inside nms()

    // Run over all the frames, runs times, and print the total time of each run
    for (int r = 0; r < runs; r++) {
        auto start = std::chrono::steady_clock::now();
        for (float* frame : frames) {
            res.clear();
#ifdef RUN_ORIG
            nms_orig(res, frame, conf, NMS_THRESHOLD);
#else
            nms(res, frame, conf, NMS_THRESHOLD);
#endif
        }
        auto end = std::chrono::steady_clock::now();
        printf("ms=%.4f\n", std::chrono::duration<double, std::milli>(end - start).count());
    }

#ifdef ALLOC_STATS
    sel_alloc_dets::report_footprint();
#endif

#ifdef VALIDATE
    // Same boxes in the same order as the original, on every frame
    std::vector<Detection> expected;
    expected.reserve(4096);
    for (size_t i = 0; i < frames.size(); i++) {
        res.clear();
        nms(res, frames[i], conf, NMS_THRESHOLD);
        expected.clear();
        nms_orig(expected, frames[i], conf, NMS_THRESHOLD);
        if (!same(res, expected)) {
            printf("validate: FAILED at frame %zu\n", i);
            return 1;
        }
    }
    printf("validate: OK\n");
#endif
    return 0;
}
