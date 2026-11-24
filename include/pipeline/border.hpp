#pragma once

#ifdef __CUDACC__
#define PIPELINE_FN __host__ __device__  // the function is available on both the CPU and the GPU
#else
#define PIPELINE_FN
#endif

// Maps any index p, also one outside 0..n-1, to the pixel that is read in its place. Pixels
// outside the image mirror the image without repeating the edge pixel (BORDER_REFLECT_101 in
// OpenCV): for the row abcdefgh, index -1 reads b and index n reads g. The mapping is periodic,
// so it also works when the filter radius is larger than the image.
PIPELINE_FN inline int reflect101(int p, int n) {
    if (n == 1) return 0;
    const int period = 2 * (n - 1);
    int v = p % period;
    if (v < 0) v += period;
    return v < n ? v : period - v;
}
