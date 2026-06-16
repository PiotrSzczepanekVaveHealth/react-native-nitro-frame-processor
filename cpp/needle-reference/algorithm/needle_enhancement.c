// needle enhancement
// 
// single grayscale frame is in colum-major order (byte).

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "needle_enhancement.h"
#include "../cmd_central.h"
#include "../../debug.h"
#include "../../probe/probe.h"
#include "../../frame/frame.h"

extern unsigned char frame_buffer[RAW_DATA_SIZE_MAX];

// testing consitency between FW version and matlat version
#if TEST_USING_EXTERNAL_DATA
    int binary_imageFile_counter = 0;
    int external_dataset_tested = 0;
    const char *const test_folders[] = {
        "testImages/",
        "testImages-03-06-2026/",
        "testImages-temp/",
        "testImages-03-03-2026/"
    };
#endif

#define NEEDLE_MOTION_MAX_ROI_ROWS (NEEDLE_MOTION_ABOVE_ROWS + NEEDLE_MOTION_BELOW_ROWS + 1)
#define NEEDLE_MOTION_MAX_ROI_COLS (2 * NEEDLE_MOTION_SEARCH_RADIUS_PX + 1)
#define NEEDLE_MOTION_MAX_ROI_PIXELS (NEEDLE_MOTION_MAX_ROI_ROWS * NEEDLE_MOTION_MAX_ROI_COLS)
#define NEEDLE_MOTION_MAX_BLOBS 16

// needle detection buffers
static unsigned char frame_buffer_roi[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static unsigned char frame_buffer_traperoid_roi[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static uint8_t I_proc[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static float profile[MAX_SAMPLE_SIZE_NEEDLE];
static float profile_s[MAX_SAMPLE_SIZE_NEEDLE];
static int row_seg_left[MAX_SAMPLE_SIZE_NEEDLE];
static int row_seg_right[MAX_SAMPLE_SIZE_NEEDLE];
static float cached_line_conf_vals[MAX_NEEDLE_SIZE_PIXELS];
static float cached_line_conf_sorted[MAX_NEEDLE_SIZE_PIXELS];
static uint8_t cached_line_conf_mask[MAX_NEEDLE_SIZE_PIXELS];
static uint8_t cached_line_conf_filled[MAX_NEEDLE_SIZE_PIXELS];

// fuse image buffer
static uint8_t tmp_morphologic[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static uint8_t tmp_fuse[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static uint8_t fused_MIP[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static uint16_t tmp16_fuse[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];

// final selected rotated image used by downstream confidence and fusion logic.
static float top_line_rot_buffers[NEEDLE_TOP_LINE_CANDIDATE_COUNT][MAX_SAMPLE_SIZE_NEEDLE * MAX_SAMPLE_SIZE_NEEDLE];
static float motion_bg[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
static float motion_diff[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];
bool motion_bg_valid = false;
static unsigned int motion_bg_H = 0;
static unsigned int motion_bg_W = 0;

// motion tip correction parameters
typedef struct
{
    bool hasTip;
    float tipX;
    bool hasLine;
    float thetaDeg;
    int rho;
    bool hasMotionTrack;
    float motionTrackX;
    float motionTrackV;
    int motionTrackCount;
    int motionMissCount;
} NeedleTipMotionState;
static NeedleTipMotionState needle_tip_motion_state = {0};

typedef struct
{
    int area;
    int width;
    int height;
    float centroidX;
    float peakSigma;
    float peakAboveThresholdSigma;
    float meanAboveThresholdSigma;
    float sumAboveThresholdSigma;
    float score;
    float selectScore;
} MotionBlob;

typedef struct
{
    int xA;
    int xB;
    int yA;
    int yB;
    int nCols;
    int nRows;
    int nPix;
    float predictionX;
    float cosA;
    float sinA;
    float cxOrig;
    float cyOrig;
    float cxRot;
    float cyRot;
} MotionHotspotRoi;

typedef struct
{
    float bg;
    float sigmaRob;
    float T;
    float scoreThr;
    float scoreSigma;
} MotionHotspotStats;

// needle detection parameters
typedef struct
{
    bool valid;
    float scoreSum;
    float theta_deg;
    int rho;
    unsigned int Ht;
    unsigned int Wt;
    int tip_x;
    int entry_x;
    float segmentMean;
    int rotBufferIdx;
} NeedleLineCandidate;

typedef struct
{
    const uint8_t *Iorig;
    unsigned int Horig;
    unsigned int Worig;
    unsigned int Hrot;
    unsigned int Wrot;
    float cosA;
    float sinA;
    float cxIn;
    float cyIn;
    float cxRot;
    float cyRot;
} DirectRotSampler;

typedef struct
{
    bool valid;
    NeedleLineResult line;
    unsigned int Hout;
    unsigned int Wout;
    unsigned int pixelCount;
    unsigned int beamCount;
    int needleLenPx;
    int lastUpdateFrame;
    bool hasHeldConfFrame;
    float heldConfFrame;
} NeedleDetectionCacheState;
static NeedleDetectionCacheState needle_detection_cache_state = {0};
static bool latest_virtual_conf_frame_valid = false;
static float latest_virtual_conf_frame = 0.0f;

// needle detection roi, needle insertion, frame size and counter parameters
static unsigned int needle_detection_physical_frame_counter = 0;
static int trapezoid_roi_zero_start_row[MAX_SCANLINES_NEEDLE];
static unsigned int trapezoid_roi_zero_len_rows[MAX_SCANLINES_NEEDLE];
static unsigned int trapezoid_roi_cache_pixelCount = 0;
static unsigned int trapezoid_roi_cache_beamCount = 0;
static int trapezoid_roi_cache_sideRight = -1;
static bool trapezoid_roi_cache_valid = false;

// needle fusion parameters
typedef struct
{
    bool valid;
    NeedleLineResult line;
    unsigned int H;
    unsigned int W;
    unsigned int rowsRot;
    unsigned int colsRot;
    int workX0;
    int workX1;
    int workY0;
    int workY1;
    int workRows;
    int morphOffsets[13];
} NeedleFusionGeometryCache;
static NeedleFusionGeometryCache fusion_geometry_cache = {0};
static uint8_t fusion_line_mask[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE];

// sigma = 5, kernel = 15 X 15, radius = 7 (symetric)
// This table is current not used.  It's kept here for possible future use (image preprocessing before needle detection)
static const uint16_t LUT_sigma5[99] =
{
10000, 9802, 9608, 9418, 9231, 9048, 8869, 8693, 8522, 8354,
8189, 8029, 7871, 7718, 7567, 7420, 7277, 7137, 6999, 6866,
6735, 6607, 6482, 6360, 6241, 6125, 6011, 5901, 5793, 5688,
5586, 5486, 5389, 5294, 5202, 5112, 5024, 4939, 4856, 4775,
4697, 4620, 4546, 4474, 4404, 4336, 4270, 4205, 4143, 4083,
4024, 3967, 3912, 3858, 3806, 3756, 3707, 3660, 3614, 3570,
3528, 3487, 3447, 3409, 3372, 3336, 3302, 3269, 3237, 3207,
3178, 3150, 3123, 3098, 3073, 3050, 3027, 3006, 2986, 2967,
2949, 2932, 2916, 2901, 2887, 2874, 2862, 2851, 2841, 2832,
2824, 2817, 2811, 2806, 2802, 2799, 2797, 2796, 2796
};

// structuring element (disk radius = 2) with a total active positions of 13
static const int8_t dx[13] = { 0, -1,0,1, -2,-1,0,1,2, -1,0,1, 0};
static const int8_t dy[13] = { -2, -1,-1,-1, 0,0,0,0,0, 1,1,1, 2};

// default needle detection parameters
NeedlePipParams needleDetectionParams =
{
    2.0f,  // theta_step_deg
    4.0f,  //theta_range_min_deg
    35.0f, //theta_range_max_deg
    1.0f,  //resize factor (currently non 1 value not working for image fusion); 
    0  // flag indicate if normalizing image image before needle detection
};

// default depth mask parameters
DepthMaskParams depthMaskParams =
{
    false, // mask_skin_layer
    8  // depth_mask_thickness_px
};

// clear cached detection and cached fusion geometry when no valid virtual line remains.
static void reset_needle_detection_cache_state(void)
{
    needle_detection_cache_state = (NeedleDetectionCacheState){0};
    fusion_geometry_cache.valid = false;
}

// check whether a cached virtual-frame line can be reused for the current physical frame.
static bool needle_detection_cache_is_usable(unsigned int pixelCount, unsigned int beamCount, int needleLenPx)
{
    return needle_detection_cache_state.valid &&
           needle_detection_cache_state.Hout > 0 &&
           needle_detection_cache_state.Wout > 0 &&
           needle_detection_cache_state.Hout <= MAX_SAMPLE_SIZE_NEEDLE &&
           needle_detection_cache_state.Wout <= MAX_SAMPLE_SIZE_NEEDLE &&
           needle_detection_cache_state.pixelCount == pixelCount &&
           needle_detection_cache_state.beamCount == beamCount &&
           needle_detection_cache_state.needleLenPx == needleLenPx;
}

// store the latest virtual-frame line and rotated dimensions for skipped physical frames.
static void update_needle_detection_cache(const NeedleLineResult *line, unsigned int Hout, unsigned int Wout, unsigned int pixelCount, unsigned int beamCount, int needleLenPx, int frameNumber_cur)
{
    needle_detection_cache_state.valid = true;
    needle_detection_cache_state.line = *line;
    needle_detection_cache_state.Hout = Hout;
    needle_detection_cache_state.Wout = Wout;
    needle_detection_cache_state.pixelCount = pixelCount;
    needle_detection_cache_state.beamCount = beamCount;
    needle_detection_cache_state.needleLenPx = needleLenPx;
    needle_detection_cache_state.lastUpdateFrame = frameNumber_cur;
    needle_detection_cache_state.hasHeldConfFrame = latest_virtual_conf_frame_valid;
    needle_detection_cache_state.heldConfFrame = latest_virtual_conf_frame_valid ? latest_virtual_conf_frame : 0.0f;
}

// copy the cached line and rotated image size into the caller output fields.
static void use_cached_needle_detection(NeedleLineResult *line, unsigned int *Hout, unsigned int *Wout)
{
    *line = needle_detection_cache_state.line;
    *Hout = needle_detection_cache_state.Hout;
    *Wout = needle_detection_cache_state.Wout;
}

// compute the tight rotated-image bounding box for one input image and angle.
static void bounding_box(unsigned int Hin, unsigned int Win, float angle_deg, unsigned int *Hout, unsigned int *Wout)
{
    if(angle_deg == 0)
    {
        *Wout = Win;
        *Hout = Hin;
        return;
    }
    float rad   = angle_deg * (float)M_PI / 180.0f;
    float cosA  = cosf(rad);
    float sinA  = sinf(rad);
    
    // Bounding box of a rotated rectangle:
    float tmpMaxW = -100000;
    float tmpMaxH = -100000;
    float tmpMinW = 100000;
    float tmpMinH = 100000;
    unsigned int cornerX[] = {0, Win-1 };
    unsigned int cornerY[] = {0, Hin-1};

    for (int i = 0; i < 2; ++ i)
    {
        for (int j = 0; j < 2; ++ j)
        {
            // axes counter clockwise rotate angle_deg (equivalent to image clockwise rotation of angle_deg)
            float outW = ((float)cornerX[i] - (Win-1)/2.0) * cosA - ((float)cornerY[j] - (Hin-1)/2.0) * sinA;
            float outH = ((float)cornerY[j] - (Hin-1)/2.0) * cosA + ((float)cornerX[i] - (Win-1)/2.0) * sinA;
            if(tmpMaxW < outW)
                tmpMaxW = outW;
            if(tmpMaxH < outH)
                tmpMaxH = outH;
            if(tmpMinW > outW)
                tmpMinW = outW;
            if(tmpMinH > outH)
                tmpMinH = outH;
        }
    }

    *Wout =(unsigned int)(ceilf(tmpMaxW) - floorf(tmpMinW) + 1);
    *Hout =(unsigned int)(ceilf(tmpMaxH) - floorf(tmpMinH) + 1);
}

// bilinear rotation about image center into a Hout x Wout sub-buffer of a Nside*Nside buffer
static void rotate_image_bilinear(const uint8_t *src, unsigned int Hin, unsigned int Win, float angle_deg, float *dst, int Nside, unsigned int Hout, unsigned int Wout)
{
    float rad  = angle_deg * (float)M_PI / 180.0f;
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    // centers in input and output (pixel-center coordinates)
    float cx_in  = (Win - 1) * 0.5f;
    float cy_in  = (Hin - 1) * 0.5f;
    float cx_out = (Wout - 1) * 0.5f;
    float cy_out = (Hout - 1) * 0.5f;

    int x, y;
    for (x = 0; x < Wout; ++x)
    {
        float x_rel = (float)x - cx_out;
        for (y = 0; y < Hout; ++y) 
        {
            float y_rel = (float)y - cy_out;

            // inverse rotation: out -> in for bilinear interpolation ( axes clockwise rotate angle_deg, (equivalent to image counter clockwise rotation of angle_deg)
            // get the nearest pixel coordinates in the original image
            float x_in = cosA * x_rel + sinA * y_rel + cx_in;
            float y_in = -sinA * x_rel + cosA * y_rel + cy_in;

            if (x_in < 0.0f || x_in > (float)(Win - 1) || y_in < 0.0f || y_in > (float)(Hin - 1))
            {
                // outside original image -> pad with 0
                dst[x * Nside + y] = 0.0f;
            } 
            else
            { // bilinear interpolation
                int x0 = (int)floorf(x_in);
                int y0 = (int)floorf(y_in);
                int x1 = x0 + 1;
                int y1 = y0 + 1;
                if (x1 >= Win)
                    x1 = Win - 1;
                if (y1 >= Hin)
                    y1 = Hin - 1;

                float dx = x_in - (float)x0;
                float dy = y_in - (float)y0;

                float v00 = (float)src[x0 * Hin + y0];
                float v10 = (float)src[x1 * Hin + y0];
                float v01 = (float)src[x0 * Hin + y1];
                float v11 = (float)src[x1 * Hin + y1];

                float v0 = v00 * (1.0f - dx) + v10 * dx;
                float v1 = v01 * (1.0f - dx) + v11 * dx;
                dst[x * Nside + y] = v0 * (1.0f - dy) + v1 * dy;
            }
        }
    }
}

// smooth the line profile with a sliding moving average used by tip detection.
static inline void smooth_profile_fast(const float *in, int n, float *out, float DC_value)
{
    int halfWin = (n / 20) < 3 ? 3 : (n / 20);
    float current_sum = 0.0f;
    int count = 0;

    // Initialize the window for the first element
    // Window starts at i=0, so it covers 0 to halfWin
    for (int k = 0; k <= halfWin && k < n; k++)
    {
        current_sum += (in[k] - DC_value);
        count++;
    }
    out[0] = current_sum / (float)count;

    // Slide the window
    for (int i = 1; i < n; i++)
    {
        int old_idx = i - halfWin - 1;
        int new_idx = i + halfWin;

        // Add the new element entering on the right
        if (new_idx < n)
        {
            current_sum += (in[new_idx] - DC_value);
            count++;
        }
        // Subtract the element leaving on the left
        if (old_idx >= 0)
        {
            current_sum -= (in[old_idx] - DC_value);
            count--;
        }

        out[i] = current_sum / (float)count;
    }
}

// depth mask (mask skin layer and bottom of the frame image)
static void depth_mask(/*const uint8_t *I,*/ unsigned int H, unsigned int W, int needle_len_px, const DepthMaskParams *params, /*uint8_t *I_depthMasked,*/ uint8_t *allowed)
{
    // initialize all allowed (column wise)
    unsigned int x;

    // mask skin (top rows)
    if (params->mask_skin_layer)
    {
        unsigned int max_row = params->depth_mask_thickness_px;
        if (max_row > H)
            max_row = H;
        for (x = 0; x < W; ++x)
            memset(&allowed[x * H], 0, max_row * sizeof(uint8_t));
    }

    // mask bottom rows based on needle length or maximum allowed rows
    int start = needle_len_px;
    int cap_start = (int)H - MAX_BOTTOM_ROW_TO_MASK;
    if (cap_start < 0)
        cap_start = 0;
    if (start < cap_start)
        start = cap_start;

    if (start < (int)H)
    {
        for (x = 0; x < W; ++x)
            memset(&allowed[x * H + start], 0, (H - start) * sizeof(uint8_t));
    }
}

// find the maximum profile peak and the last local-peak location for fixed-threshold tip picking.
static void find_max_peak_lastPkLoc(const float *vals, int N, float *max_pk_val, int *last_pk_loc)
{
    *max_pk_val = 0.0f;
    *last_pk_loc = -1;

    if (vals == NULL || N < 3)
        return;

    for (int i = 1; i < N - 1; ++i)
    {
        if ((vals[i] > vals[i - 1]) && (vals[i] >= vals[i + 1]))
        {
            if (vals[i] > *max_pk_val)
                *max_pk_val = vals[i];

            *last_pk_loc = i;
        }
    }
}

// initialize the top-line candidate table before scanning theta/rho combinations.
static void init_line_candidates(NeedleLineCandidate *cands)
{
    for (int i = 0; i < NEEDLE_TOP_LINE_CANDIDATE_COUNT; ++i)
    {
        cands[i].valid = false;
        cands[i].scoreSum = -1.0e30f;
        cands[i].theta_deg = 0.0f;
        cands[i].rho = 0;
        cands[i].Ht = 0;
        cands[i].Wt = 0;
        cands[i].tip_x = 0;
        cands[i].entry_x = 0;
        cands[i].segmentMean = -1.0e30f;
        cands[i].rotBufferIdx = -1;
    }
}

// return the reusable rotated-image buffer assigned to a top-line candidate.
static float *top_line_rot_buffer(int idx)
{
    return top_line_rot_buffers[idx];
}
// reset profile-tip and motion-tip temporal state.
void reset_tip_motion_state(void)
{
    needle_tip_motion_state = (NeedleTipMotionState){0};
}

// compute wrapped absolute angle difference in degrees.
static float angle_difference_deg(float a, float b)
{
    float d = fmodf((a - b) + 180.0f, 360.0f);
    if (d < 0.0f)
        d += 360.0f;
    return fabsf(d - 180.0f);
}

// map one point from rotated coordinates back into original image coordinates.
static void rotated_point_to_original(unsigned int H, unsigned int W, unsigned int Hrot, unsigned int Wrot, float theta_deg, float xrot, float yrot, float *xorig, float *yorig)
{
    float rad = theta_deg * (float)M_PI / 180.0f;
    float cosA = cosf(rad);
    float sinA = sinf(rad);
    float cx_in = ((float)W - 1.0f) * 0.5f;
    float cy_in = ((float)H - 1.0f) * 0.5f;
    float cx_rot = ((float)Wrot - 1.0f) * 0.5f;
    float cy_rot = ((float)Hrot - 1.0f) * 0.5f;
    float xr = xrot - cx_rot;
    float yr = yrot - cy_rot;

    *xorig = cosA * xr + sinA * yr + cx_in;
    *yorig = -sinA * xr + cosA * yr + cy_in;
}

// map original-frame corners into the rotated image to define the valid polygon.
static void rotated_original_frame_corners(unsigned int H, unsigned int W, unsigned int Hrot, unsigned int Wrot, float theta_deg, float cornerX[4], float cornerY[4])
{
    float rad = theta_deg * (float)M_PI / 180.0f;
    float cosA = cosf(rad);
    float sinA = sinf(rad);
    float cx_in = ((float)W - 1.0f) * 0.5f;
    float cy_in = ((float)H - 1.0f) * 0.5f;
    float cx_rot = ((float)Wrot - 1.0f) * 0.5f;
    float cy_rot = ((float)Hrot - 1.0f) * 0.5f;
    const float origX[4] = {0.0f, (float)W - 1.0f, (float)W - 1.0f, 0.0f};
    const float origY[4] = {0.0f, 0.0f, (float)H - 1.0f, (float)H - 1.0f};

    for (int i = 0; i < 4; ++i)
    {
        float xr = origX[i] - cx_in;
        float yr = origY[i] - cy_in;
        cornerX[i] = cosA * xr - sinA * yr + cx_rot;
        cornerY[i] = sinA * xr + cosA * yr + cy_rot;
    }
}

// intersect a rotated row with the original-frame polygon and return valid x bounds.
static bool rotated_row_x_bounds_from_corners(const float cornerX[4], const float cornerY[4], unsigned int Wrot, int rho, int *x_left, int *x_right)
{
    float xints[8];
    int nints = 0;
    float yline = (float)rho;

    for (int i = 0; i < 4; ++i)
    {
        int j = (i + 1) & 3;
        float x1 = cornerX[i];
        float y1 = cornerY[i];
        float x2 = cornerX[j];
        float y2 = cornerY[j];
        float minY = (y1 < y2) ? y1 : y2;
        float maxY = (y1 > y2) ? y1 : y2;

        if (yline < minY - 0.001f || yline > maxY + 0.001f)
            continue;

        if (fabsf(y2 - y1) < 1.0e-6f)
        {
            if (fabsf(yline - y1) < 0.001f && nints <= 6)
            {
                xints[nints++] = x1;
                xints[nints++] = x2;
            }
        }
        else
        {
            float t = (yline - y1) / (y2 - y1);
            if (t >= -0.001f && t <= 1.001f && nints < 8)
                xints[nints++] = x1 + t * (x2 - x1);
        }
    }

    if (nints < 2)
        return false;

    float minX = xints[0];
    float maxX = xints[0];
    for (int i = 1; i < nints; ++i)
    {
        if (xints[i] < minX)
            minX = xints[i];
        if (xints[i] > maxX)
            maxX = xints[i];
    }

    int left = (int)ceilf(minX - 0.001f);
    int right = (int)floorf(maxX + 0.001f);
    left = clampi(left, 0, (int)Wrot - 1);
    right = clampi(right, 0, (int)Wrot - 1);

    if (right < left)
        return false;

    *x_left = left;
    *x_right = right;
    return true;
}

// convenience wrapper to compute valid x bounds for a rotated row.
static bool rotated_row_original_x_bounds(unsigned int H, unsigned int W, unsigned int Hrot, unsigned int Wrot, float theta_deg, int rho, int *x_left, int *x_right)
{
    float cornerX[4];
    float cornerY[4];
    rotated_original_frame_corners(H, W, Hrot, Wrot, theta_deg, cornerX, cornerY);
    return rotated_row_x_bounds_from_corners(cornerX, cornerY, Wrot, rho, x_left, x_right);
}

// reject geometrically implausible lines whose entry maps too deep in the original frame.
static bool entry_is_too_deep(unsigned int H, unsigned int W, unsigned int Hrot, unsigned int Wrot, float theta_deg, int entry_x, int entry_y, float *entryOrigY, float *entryLimitY)
{
    float xo, yo;
    rotated_point_to_original(H, W, Hrot, Wrot, theta_deg, (float)entry_x, (float)entry_y, &xo, &yo);
    (void)xo;

    float limit = NEEDLE_MAX_ENTRY_ORIG_Y_FRACTION * (float)H;
    if (limit > (float)NEEDLE_MAX_ENTRY_ORIG_Y_PX)
        limit = (float)NEEDLE_MAX_ENTRY_ORIG_Y_PX;

    if (entryOrigY)
        *entryOrigY = yo;
    if (entryLimitY)
        *entryLimitY = limit;

    return yo > limit;
}

// insert one candidate into the top-N metadata list ordered by raw line score.
static void insert_top_line_candidate(NeedleLineCandidate *cands, NeedleLineCandidate cand)
{
    int worstIdx = 0;
    float worstScore = cands[0].scoreSum;

    for (int i = 1; i < NEEDLE_TOP_LINE_CANDIDATE_COUNT; ++i)
    {
        if (cands[i].scoreSum < worstScore)
        {
            worstScore = cands[i].scoreSum;
            worstIdx = i;
        }
    }

    if (cand.scoreSum <= worstScore)
        return;

    cand.rotBufferIdx = -1;
    cands[worstIdx] = cand;
    cands[worstIdx].valid = true;

    for (int i = 0; i < NEEDLE_TOP_LINE_CANDIDATE_COUNT - 1; ++i)
    {
        for (int j = i + 1; j < NEEDLE_TOP_LINE_CANDIDATE_COUNT; ++j)
        {
            if (cands[j].scoreSum > cands[i].scoreSum)
            {
                NeedleLineCandidate tmp = cands[i];
                cands[i] = cands[j];
                cands[j] = tmp;
            }
        }
    }
}

// precompute valid x integration bounds for every rotated row at one theta.
static void prepare_rotated_row_segments(const float cornerX[4], const float cornerY[4], unsigned int Hrot, unsigned int Wrot, unsigned int needle_len_rot)
{
    int maxRows = (Hrot > MAX_SAMPLE_SIZE_NEEDLE) ? MAX_SAMPLE_SIZE_NEEDLE : (int)Hrot;

    for (int y = 0; y < maxRows; ++y)
    {
        int x_left_bound = 0;
        int x_right_bound = 0;
        row_seg_left[y] = 1;
        row_seg_right[y] = 0;

        if (!rotated_row_x_bounds_from_corners(cornerX, cornerY, Wrot, y, &x_left_bound, &x_right_bound))
            continue;

        if (NEEDLE_INSERSION_SIDE_RIGHT)
        {
            int x_right = x_right_bound;
            int x_left = x_right - (int)needle_len_rot + 1;
            if (x_left < x_left_bound)
                x_left = x_left_bound;
            row_seg_left[y] = x_left;
            row_seg_right[y] = x_right;
        }
        else
        {
            int x_left = x_left_bound;
            int x_right = x_left + (int)needle_len_rot - 1;
            if (x_right > x_right_bound)
                x_right = x_right_bound;
            row_seg_left[y] = x_left;
            row_seg_right[y] = x_right;
        }
    }
}

// build a rotated-row profile without full-frame rotation by forward-projecting original pixels.
static void accumulate_rotated_profile_forward(const uint8_t *I, unsigned int H, unsigned int W, float theta_deg, unsigned int Hrot, unsigned int Wrot, unsigned int needle_len_rot, float *profile_out)
{
    memset(profile_out, 0, Hrot * sizeof(float));

    float cornerX[4];
    float cornerY[4];
    rotated_original_frame_corners(H, W, Hrot, Wrot, theta_deg, cornerX, cornerY);
    prepare_rotated_row_segments(cornerX, cornerY, Hrot, Wrot, needle_len_rot);

    float rad = theta_deg * (float)M_PI / 180.0f;
    float cosA = cosf(rad);
    float sinA = sinf(rad);
    float cx_in = ((float)W - 1.0f) * 0.5f;
    float cy_in = ((float)H - 1.0f) * 0.5f;
    float cx_rot = ((float)Wrot - 1.0f) * 0.5f;
    float cy_rot = ((float)Hrot - 1.0f) * 0.5f;

    for (unsigned int x = 0; x < W; ++x)
    {
        float xr = (float)x - cx_in;
        float xrot = cosA * xr + sinA * cy_in + cx_rot;
        float yrot = sinA * xr - cosA * cy_in + cy_rot;

        for (unsigned int y = 0; y < H; ++y)
        {
            uint8_t pix = I[(size_t)x * H + y];
            if (pix != 0U && yrot >= 0.0f && yrot <= (float)(Hrot - 1U))
            {
#if NEEDLE_DETECTION_ACCUM_USE_Y_BILINEAR
                int r0 = (int)yrot;
                float wy1 = yrot - (float)r0;
                float wy0 = 1.0f - wy1;

                if (row_seg_left[r0] <= row_seg_right[r0] && xrot >= (float)row_seg_left[r0] && xrot <= (float)row_seg_right[r0])
                    profile_out[r0] += (float)pix * wy0;

                int r1 = r0 + 1;
                if (r1 < (int)Hrot && row_seg_left[r1] <= row_seg_right[r1] && xrot >= (float)row_seg_left[r1] && xrot <= (float)row_seg_right[r1])
                    profile_out[r1] += (float)pix * wy1;
#else
                int r = (int)(yrot + 0.5f);
                if (r >= 0 && r < (int)Hrot && row_seg_left[r] <= row_seg_right[r] &&
                    xrot >= (float)row_seg_left[r] && xrot <= (float)row_seg_right[r])
                {
                    profile_out[r] += (float)pix;
                }
#endif
            }

            xrot -= sinA;
            yrot += cosA;
        }
    }

#if NEEDLE_LINE_HALF_BAND > 0
    for (unsigned int r = 0; r < Hrot; ++r)
    {
        float sum = 0.0f;
        int count = 0;
        for (int dy = -NEEDLE_LINE_HALF_BAND; dy <= NEEDLE_LINE_HALF_BAND; ++dy)
        {
            int rr = (int)r + dy;
            if (rr >= 0 && rr < (int)Hrot && row_seg_left[rr] <= row_seg_right[rr])
            {
                sum += profile_out[rr];
                count++;
            }
        }
        profile_s[r] = (count > 0) ? sum / (float)count : 0.0f;
    }
    memcpy(profile_out, profile_s, Hrot * sizeof(float));
#endif
}

// bilinear sample from the original image at floating-point original-frame coordinates.
static float sample_original_bilinear_u8(const uint8_t *Iorig, unsigned int Horig, unsigned int Worig, float xOrig, float yOrig)
{
    if (xOrig < 0.0f || xOrig > (float)(Worig - 1) || yOrig < 0.0f || yOrig > (float)(Horig - 1))
        return 0.0f;

    int x0 = (int)floorf(xOrig);
    int y0 = (int)floorf(yOrig);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    if (x1 >= (int)Worig) x1 = (int)Worig - 1;
    if (y1 >= (int)Horig) y1 = (int)Horig - 1;

    float dx = xOrig - (float)x0;
    float dy = yOrig - (float)y0;
    float v00 = (float)Iorig[x0 * Horig + y0];
    float v10 = (float)Iorig[x1 * Horig + y0];
    float v01 = (float)Iorig[x0 * Horig + y1];
    float v11 = (float)Iorig[x1 * Horig + y1];
    float v0 = v00 * (1.0f - dx) + v10 * dx;
    float v1 = v01 * (1.0f - dx) + v11 * dx;

    return v0 * (1.0f - dy) + v1 * dy;
}

// precompute constants for direct rotated-coordinate sampling.
static void init_direct_rot_sampler(DirectRotSampler *s, const uint8_t *Iorig, unsigned int Horig, unsigned int Worig, unsigned int Hrot, unsigned int Wrot, float theta_deg)
{
    float rad = theta_deg * (float)M_PI / 180.0f;
    s->Iorig = Iorig;
    s->Horig = Horig;
    s->Worig = Worig;
    s->Hrot = Hrot;
    s->Wrot = Wrot;
    s->cosA = cosf(rad);
    s->sinA = sinf(rad);
    s->cxIn = ((float)Worig - 1.0f) * 0.5f;
    s->cyIn = ((float)Horig - 1.0f) * 0.5f;
    s->cxRot = ((float)Wrot - 1.0f) * 0.5f;
    s->cyRot = ((float)Hrot - 1.0f) * 0.5f;
}

// sample the image value that would appear at one rotated-frame coordinate, without constructing the full rotated frame.
static float sample_rotated_coord_direct(const DirectRotSampler *s, float xrot, float yrot)
{
    float xRel = xrot - s->cxRot;
    float yRel = yrot - s->cyRot;
    float xOrig = s->cosA * xRel + s->sinA * yRel + s->cxIn;
    float yOrig = -s->sinA * xRel + s->cosA * yRel + s->cyIn;

    return sample_original_bilinear_u8(s->Iorig, s->Horig, s->Worig, xOrig, yOrig);
}

// direct version of find_tip_x_along_best_line(). It samples only the candidate band from the original frame and runs the same profile smoothing/thresholding.
static int find_tip_x_along_best_line_direct(const DirectRotSampler *sampler, int rho)
{
    int tip_x = -1;
    float best_line_sum = 0.0f;
    float best_line_DC_value = 0.0f;

    unsigned int Hrot = sampler->Hrot;
    unsigned int Wrot = sampler->Wrot;
    rho = clampi(rho, 0, (int)Hrot - 1);

    for (int x = 0; x < (int)Wrot; ++x)
    {
        float bandSum = 0.0f;
        int bandCount = 0;
        for (int dy = -NEEDLE_LINE_HALF_BAND; dy <= NEEDLE_LINE_HALF_BAND; ++dy)
        {
            int y = rho + dy;
            if (y >= 0 && y < (int)Hrot)
            {
                bandSum += sample_rotated_coord_direct(sampler, (float)x, (float)y);
                bandCount++;
            }
        }

        profile[x] = (bandCount > 0) ? bandSum / (float)bandCount : 0.0f;
        best_line_sum += profile[x];
    }

    best_line_DC_value = best_line_sum / (float)Wrot;
    smooth_profile_fast(profile, Wrot, profile_s, best_line_DC_value);

    float maxv = -1.0e30f;
    int max_idx = -1;
    find_max_peak_lastPkLoc(profile_s, Wrot, &maxv, &max_idx);

    if (maxv > 0.0f)
    {
        float inv = 1.0f / maxv;
        for (int i = 0; i < (int)Wrot; ++i)
            profile_s[i] *= inv;
    }

    for (int i = max_idx - 1; i > 1; --i)
    {
        if (profile_s[i] > NEEDLE_ENDPOINTS_THRESHOLD && profile_s[i - 1] < NEEDLE_ENDPOINTS_THRESHOLD)
        {
            tip_x = i;
            break;
        }
    }

    if (max_idx < 0 || tip_x < 0)
        tip_x = 0;

    return tip_x;
}

// score a candidate segment by direct band sampling instead of requiring a full rotated image.
static float line_segment_band_mean_direct(const DirectRotSampler *sampler, int rho, int tip_x, int entry_x, int *segmentLength)
{
    unsigned int Hrot = sampler->Hrot;
    unsigned int Wrot = sampler->Wrot;
    int x0 = clampi((tip_x < entry_x) ? tip_x : entry_x, 0, (int)Wrot - 1);
    int x1 = clampi((tip_x > entry_x) ? tip_x : entry_x, 0, (int)Wrot - 1);

    if (x1 < x0)
    {
        *segmentLength = 0;
        return -1.0e30f;
    }

    *segmentLength = x1 - x0 + 1;

    float sum = 0.0f;
    int count = 0;
    for (int x = x0; x <= x1; ++x)
    {
        for (int dy = -NEEDLE_LINE_HALF_BAND; dy <= NEEDLE_LINE_HALF_BAND; ++dy)
        {
            int y = rho + dy;
            if (y >= 0 && y < (int)Hrot)
            {
                float v = sample_rotated_coord_direct(sampler, (float)x, (float)y);
                if (v > 0.0f)
                {
                    sum += v;
                    count++;
                }
            }
        }
    }

    if (count <= 0)
        return -1.0e30f;

    return sum / (float)count;
}

// update the EMA background and absolute-difference image used by motion tip correction.
static void update_motion_difference_image(const uint8_t *Icur, unsigned int H, unsigned int W)
{
    unsigned int N = H * W;

    if (!motion_bg_valid || motion_bg_H != H || motion_bg_W != W)
    {
        for (unsigned int i = 0; i < N; ++i)
        {
            motion_bg[i] = (float)Icur[i];
            motion_diff[i] = 0.0f;
        }
        motion_bg_valid = true;
        motion_bg_H = H;
        motion_bg_W = W;
        return;
    }

    for (unsigned int i = 0; i < N; ++i)
    {
        float cur = (float)Icur[i];
        motion_bg[i] = NEEDLE_MOTION_EMA_ALPHA * cur + (1.0f - NEEDLE_MOTION_EMA_ALPHA) * motion_bg[i];
        motion_diff[i] = fabsf(cur - motion_bg[i]);
    }
}

// swap function for in-place quickselect median calculation.
static void swap_float(float *a, float *b)
{
    float tmp = *a;
    *a = *b;
    *b = tmp;
}

// three-way partition handles repeated values efficiently, which is commonly used in quiet diff ROIs (values are close to each other).
static void partition3_float(float *a, int left, int right, int pivotIndex, int *ltOut, int *gtOut)
{
    float pivot = a[pivotIndex];
    int lt = left;
    int i = left;
    int gt = right;

    while (i <= gt)
    {
        if (a[i] < pivot)
        {
            swap_float(&a[lt], &a[i]);
            lt++;
            i++;
        }
        else if (a[i] > pivot)
        {
            swap_float(&a[i], &a[gt]);
            gt--;
        }
        else
            i++;
    }

    *ltOut = lt;
    *gtOut = gt;
}

// select the kth smallest item in-place.
static float quickselect_float(float *a, int n, int k)
{
    int left = 0;
    int right = n - 1;

    while (left < right)
    {
        int mid = left + ((right - left) >> 1);

        // Median-of-three pivot selection reduces bad partitions on nearly sorted data.
        if (a[right] < a[left]) swap_float(&a[right], &a[left]);
        if (a[mid] < a[left]) swap_float(&a[mid], &a[left]);
        if (a[right] < a[mid]) swap_float(&a[right], &a[mid]);

        int lt = left;
        int gt = right;
        partition3_float(a, left, right, mid, &lt, &gt);

        if (k >= lt && k <= gt)
            return a[k];
        if (k < lt)
            right = lt - 1;
        else
            left = gt + 1;
    }

    return a[left];
}

// compute the median using quickselect instead of full insertion sort.
static float median_float(float *a, int n)
{
    if (n <= 0)
        return 0.0f;

    if ((n & 1) != 0)
        return quickselect_float(a, n, n / 2);

    float lo = quickselect_float(a, n, n / 2 - 1);
    float hi = quickselect_float(a, n, n / 2);
    return 0.5f * (lo + hi);
}

// find tip x along the best needle line
static int find_tip_x_along_best_line(float *Irot_best, unsigned int best_Ht, unsigned int best_Wt, unsigned int stride_rot, int best_rho)
{
    int tip_x = -1;
    float best_line_sum = 0.0f;
    float best_line_DC_value = 0.0f;

    best_rho = clampi(best_rho, 0, (int)best_Ht - 1);

    // Sample along a small line band, matching the MATLAB prototype.
    for (int x = 0; x < best_Wt; ++x)
    {
        float bandSum = 0.0f;
        int bandCount = 0;
        for (int dy = -NEEDLE_LINE_HALF_BAND; dy <= NEEDLE_LINE_HALF_BAND; ++dy)
        {
            int y = best_rho + dy;
            if (y >= 0 && y < (int)best_Ht)
            {
                bandSum += Irot_best[x * stride_rot + y];
                bandCount++;
            }
        }
        profile[x] = (bandCount > 0) ? bandSum / (float)bandCount : 0.0f;
        best_line_sum += profile[x];
    }
    best_line_DC_value = best_line_sum /(float)best_Wt;

    smooth_profile_fast(profile, best_Wt, profile_s, best_line_DC_value);
    
    float maxv = -1.0e30f;
    int max_idx = -1;
    find_max_peak_lastPkLoc(profile_s, best_Wt, &maxv, &max_idx);

    // normalize
    if (maxv > 0.0f)
    {
        float inv = 1.0f / maxv;
        for (int i = 0; i < best_Wt; ++i)
            profile_s[i] *= inv;
    }
    
    for (int i = max_idx -1 ; i > 1; --i) 
    {
        if (profile_s[i] > NEEDLE_ENDPOINTS_THRESHOLD && profile_s[i-1] < NEEDLE_ENDPOINTS_THRESHOLD)
        {
                tip_x = i;
                break;
        }
    }

    if (max_idx < 0 || tip_x < 0)
        tip_x = 0;

    return tip_x;
}
// refine the detected needle line by searching for the maximum profile sum in a local vertical window (defined by REFINEMENT_OFFSET_for_DETECTED_NEEDLE) around the detected tip y coordinate. The search is performed within the x range defined by the detected entry and tip x coordinates. The refined tip y coordinate is updated in place in the input detected_needle_line struct.
// Optionally refine rho and recompute the band-profile tip on the selected line.
void refine_detected_needle_line(float *Irot_best, unsigned int best_Ht, unsigned int best_Wt, unsigned int stride_rot, NeedleLineResult *detected_needle_line)
{
    int starti = detected_needle_line->tip_y - REFINEMENT_OFFSET_for_DETECTED_NEEDLE;
    if (starti < 0)
        starti = 0;

    int endi = detected_needle_line->tip_y + REFINEMENT_OFFSET_for_DETECTED_NEEDLE;
    if (endi >= best_Ht)
        endi = best_Ht - 1;

    int num_of_scanlines = endi - starti + 1;
    if (num_of_scanlines <= 0) 
        return; // safety check

    float sum_profile[2*REFINEMENT_OFFSET_for_DETECTED_NEEDLE + 1];
    int num_nonzero_pixels[2*REFINEMENT_OFFSET_for_DETECTED_NEEDLE + 1];
    memset(sum_profile, 0, sizeof(sum_profile));
    memset(num_nonzero_pixels, 0, sizeof(num_nonzero_pixels));

    int x0 = detected_needle_line->tip_x;
    int x1 = detected_needle_line->entry_x;
    if (x0 > x1)
    {
        int t = x0;
        x0 = x1;
        x1 = t;
    }

    x0 = clampi(x0, 0, best_Wt - 1);
    x1 = clampi(x1, 0, best_Wt - 1);

    float best_mean = -1.0f;
    int best_k = detected_needle_line->best_rho;

    for (int x = x0; x <= x1; ++x)
    {
        for (int k = starti; k <= endi; ++k)
        {
            for (int dy = -NEEDLE_LINE_HALF_BAND; dy <= NEEDLE_LINE_HALF_BAND; ++dy)
            {
                int yy = k + dy;
                if (yy >= 0 && yy < (int)best_Ht)
                {
                    float v = Irot_best[x * stride_rot + yy];
                    if (v > 0.0f)
                    {
                        sum_profile[k - starti] += v;
                        num_nonzero_pixels[k - starti] ++;
                    }
                }
            }
        }
    }

    for (int k = starti; k <= endi; ++k)
    {
        if (num_nonzero_pixels[k - starti] > 0)
        {
            float mean = sum_profile[k - starti] / (float)num_nonzero_pixels[k - starti]; 
            if (mean > best_mean)
            {
                best_mean = mean;
                best_k = k;
            }
        }
    }
    if (detected_needle_line->best_rho != best_k)
    {
        LogDebug("Refined detected needle line tip y from %d to %d\n", detected_needle_line->best_rho, best_k);
    
        detected_needle_line->tip_y = best_k;
        detected_needle_line->entry_y = best_k;
        detected_needle_line->best_rho = best_k;

       // refine needle tip
       detected_needle_line->tip_x = find_tip_x_along_best_line(Irot_best, best_Ht, best_Wt, stride_rot, best_k);
    }
}

// evaluate one top candidate by sampling only the candidate line band directly from the original image. This avoids full-frame rotation.
static bool evaluate_line_candidate_by_direct_sampling(const uint8_t *Iorig, unsigned int Horig, unsigned int Worig, unsigned int Hrot, unsigned int Wrot, float theta_deg,
                                                       const float cornerX[4], const float cornerY[4], int rho, float scoreSum, NeedleLineCandidate *candOut)
{
    int x_left_bound = 0;
    int x_right_bound = 0;
    if (!rotated_row_x_bounds_from_corners(cornerX, cornerY, Wrot, rho, &x_left_bound, &x_right_bound))
        return false;

    int entryEval = NEEDLE_INSERSION_SIDE_RIGHT ? x_right_bound : x_left_bound;
    int rhoEval = rho;
    DirectRotSampler sampler;
    init_direct_rot_sampler(&sampler, Iorig, Horig, Worig, Hrot, Wrot, theta_deg);
    int tipEval = find_tip_x_along_best_line_direct(&sampler, rhoEval);

#if NEEDLE_TOP_LINE_REFINE_RHO_BEFORE_RESCORE
    int startRow = rhoEval - REFINEMENT_OFFSET_for_DETECTED_NEEDLE;
    int endRow = rhoEval + REFINEMENT_OFFSET_for_DETECTED_NEEDLE;
    startRow = clampi(startRow, 0, (int)Hrot - 1);
    endRow = clampi(endRow, 0, (int)Hrot - 1);

    int bestRow = rhoEval;
    float bestMean = -1.0e30f;
    for (int rr = startRow; rr <= endRow; ++rr)
    {
        int segLen = 0;
        float mean = line_segment_band_mean_direct(&sampler, rr, tipEval, entryEval, &segLen);
        if (segLen >= NEEDLE_TOP_LINE_MIN_SEGMENT_PIXELS && mean > bestMean)
        {
            bestMean = mean;
            bestRow = rr;
        }
    }

    rhoEval = bestRow;
    tipEval = find_tip_x_along_best_line_direct(&sampler, rhoEval);
    if (rotated_row_x_bounds_from_corners(cornerX, cornerY, Wrot, rhoEval, &x_left_bound, &x_right_bound))
        entryEval = NEEDLE_INSERSION_SIDE_RIGHT ? x_right_bound : x_left_bound;
#endif

    int segmentLength = 0;
    float segmentMean = line_segment_band_mean_direct(&sampler, rhoEval, tipEval, entryEval, &segmentLength);
    if (segmentLength < NEEDLE_TOP_LINE_MIN_SEGMENT_PIXELS)
        return false;

#if NEEDLE_REJECT_ENTRY_TOO_DEEP
    if (entry_is_too_deep(Horig, Worig, Hrot, Wrot, theta_deg, entryEval, rhoEval, NULL, NULL))
        return false;
#endif

    candOut->valid = true;
    candOut->scoreSum = scoreSum;
    candOut->theta_deg = theta_deg;
    candOut->rho = rhoEval;
    candOut->Ht = Hrot;
    candOut->Wt = Wrot;
    candOut->tip_x = tipEval;
    candOut->entry_x = entryEval;
    candOut->segmentMean = segmentMean;
    candOut->rotBufferIdx = -1;
    return true;
}

// age the motion-tip tracker when no valid compact hotspot is found.
static void mark_motion_track_miss(void)
{
    if (needle_tip_motion_state.hasMotionTrack)
    {
        needle_tip_motion_state.motionMissCount++;
        if (needle_tip_motion_state.motionMissCount > NEEDLE_MOTION_TRACK_MAX_MISS_FRAMES)
        {
            needle_tip_motion_state.hasMotionTrack = false;
            needle_tip_motion_state.motionTrackX = 0.0f;
            needle_tip_motion_state.motionTrackV = 0.0f;
            needle_tip_motion_state.motionTrackCount = 0;
        }
    }
}

// store a valid motion hotspot in the temporal tracker. 
static void update_motion_track_hit(float tipMotion, float newVelocity, int newTrackCount)
{
    needle_tip_motion_state.hasMotionTrack = true;
    needle_tip_motion_state.motionTrackX = tipMotion;
    needle_tip_motion_state.motionTrackV = newVelocity;
    needle_tip_motion_state.motionTrackCount = newTrackCount;
    needle_tip_motion_state.motionMissCount = 0;
}

// define the small rotated-coordinate search ROI around the profile tip.
static bool setup_motion_hotspot_roi(int Horig, int Worig, int Hrot, int Wrot, float theta_deg, int rho, int tipOriginal, int entryX, MotionHotspotRoi *roiInfo)
{
    float predictionX = (float)tipOriginal;
    if (needle_tip_motion_state.hasMotionTrack)
        predictionX = needle_tip_motion_state.motionTrackX + needle_tip_motion_state.motionTrackV;
    else if (needle_tip_motion_state.hasTip)
        predictionX = needle_tip_motion_state.tipX;
    predictionX = clampf(predictionX, 0.0f, (float)(Wrot - 1));

    int xA = clampi(tipOriginal - NEEDLE_MOTION_SEARCH_RADIUS_PX, 0, Wrot - 1);
    int xB = clampi(tipOriginal + NEEDLE_MOTION_SEARCH_RADIUS_PX, 0, Wrot - 1);

    if (NEEDLE_INSERSION_SIDE_RIGHT)
        xB = clampi(xB, 0, entryX);
    else
        xA = clampi(xA, entryX, Wrot - 1);

    int yA = clampi(rho - NEEDLE_MOTION_ABOVE_ROWS, 0, Hrot - 1);
    int yB = clampi(rho + NEEDLE_MOTION_BELOW_ROWS, 0, Hrot - 1);
    if (xB < xA || yB < yA)
        return false;

    int nCols = xB - xA + 1;
    int nRows = yB - yA + 1;
    if (nCols < 3 || nCols > NEEDLE_MOTION_MAX_ROI_COLS || nRows > NEEDLE_MOTION_MAX_ROI_ROWS)
        return false;

    float rad = theta_deg * (float)M_PI / 180.0f;
    roiInfo->xA = xA;
    roiInfo->xB = xB;
    roiInfo->yA = yA;
    roiInfo->yB = yB;
    roiInfo->nCols = nCols;
    roiInfo->nRows = nRows;
    roiInfo->nPix = nCols * nRows;
    roiInfo->predictionX = predictionX;
    roiInfo->cosA = cosf(rad);
    roiInfo->sinA = sinf(rad);
    roiInfo->cxOrig = ((float)Worig - 1.0f) * 0.5f;
    roiInfo->cyOrig = ((float)Horig - 1.0f) * 0.5f;
    roiInfo->cxRot = ((float)Wrot - 1.0f) * 0.5f;
    roiInfo->cyRot = ((float)Hrot - 1.0f) * 0.5f;
    return true;
}

// sample the rotated ROI from the unrotated difference image using nearest neighbor.
static void sample_motion_roi_from_diff(const float *Dorig, int Horig, int Worig, const MotionHotspotRoi *roiInfo, float *roiRaw)
{
    for (int ry = 0; ry < roiInfo->nRows; ++ry)
    {
        float xRel0 = (float)roiInfo->xA - roiInfo->cxRot;
        float yRel = (float)(roiInfo->yA + ry) - roiInfo->cyRot;
        float xOrig = roiInfo->cosA * xRel0 + roiInfo->sinA * yRel + roiInfo->cxOrig;
        float yOrig = -roiInfo->sinA * xRel0 + roiInfo->cosA * yRel + roiInfo->cyOrig;

        for (int cx = 0; cx < roiInfo->nCols; ++cx)
        {
            int idx = ry * roiInfo->nCols + cx;
            int xo = (int)floorf(xOrig + 0.5f);
            int yo = (int)floorf(yOrig + 0.5f);

            if (xo >= 0 && xo < Worig && yo >= 0 && yo < Horig)
                roiRaw[idx] = Dorig[xo * Horig + yo];
            else
                roiRaw[idx] = 0.0f;

            xOrig += roiInfo->cosA;
            yOrig -= roiInfo->sinA;
        }
    }
}

// cheap box smoothing and scratch initialization for hotspot detection.
static void smooth_motion_roi_and_clear_scratch(const MotionHotspotRoi *roiInfo, const float *roiRaw, float *roi, float *roiVals, float *hot, uint8_t *bw, uint8_t *visited, float *score)
{
    for (int cx = 0; cx < roiInfo->nCols; ++cx)
    {
        score[cx] = 0.0f;
        for (int ry = 0; ry < roiInfo->nRows; ++ry)
        {
            int idx = ry * roiInfo->nCols + cx;
            float boxSum = 0.0f;
            int boxCount = 0;

            for (int dc = -NEEDLE_MOTION_BOX_RADIUS; dc <= NEEDLE_MOTION_BOX_RADIUS; ++dc)
            {
                int cc = clampi(cx + dc, 0, roiInfo->nCols - 1);
                for (int dr = -NEEDLE_MOTION_BOX_RADIUS; dr <= NEEDLE_MOTION_BOX_RADIUS; ++dr)
                {
                    int rr = clampi(ry + dr, 0, roiInfo->nRows - 1);
                    boxSum += roiRaw[rr * roiInfo->nCols + cc];
                    boxCount++;
                }
            }

            roi[idx] = (boxCount > 0) ? boxSum / (float)boxCount : roiRaw[idx];
            roiVals[idx] = roi[idx];
            hot[idx] = 0.0f;
            bw[idx] = 0;
            visited[idx] = 0;
        }
    }
}

// estimate local background/noise and build the bright-pixel threshold.
static bool estimate_motion_hotspot_threshold(const MotionHotspotRoi *roiInfo, const float *roi, float *roiVals, float *devVals, MotionHotspotStats *stats)
{
    stats->bg = median_float(roiVals, roiInfo->nPix);
    for (int i = 0; i < roiInfo->nPix; ++i)
        devVals[i] = fabsf(roi[i] - stats->bg);

    stats->sigmaRob = 1.4826f * median_float(devVals, roiInfo->nPix);
    if (stats->sigmaRob <= 1.0e-6f)
    {
        float sum = 0.0f;
        float sum2 = 0.0f;
        for (int i = 0; i < roiInfo->nPix; ++i)
        {
            sum += roi[i];
            sum2 += roi[i] * roi[i];
        }
        float mean = sum / (float)roiInfo->nPix;
        stats->sigmaRob = sqrtf(fmaxf(0.0f, sum2 / (float)roiInfo->nPix - mean * mean));
    }
    stats->sigmaRob = fmaxf(stats->sigmaRob, 1.0e-3f);

    float peakVal = roi[0];
    for (int i = 1; i < roiInfo->nPix; ++i)
    {
        if (roi[i] > peakVal)
            peakVal = roi[i];
    }

    float amp = peakVal - stats->bg;
    //LogInfo("\\CLI amp = %f\n", amp);
    if (amp <= 1.0e-6f)
        return false;

    stats->T = fmaxf(stats->bg + NEEDLE_MOTION_ROBUST_KSIGMA * stats->sigmaRob,
                     stats->bg + NEEDLE_MOTION_PEAK_FRAC * amp);
    return true;
}

// convert the smoothed ROI into a binary hot mask and a 1D x-score.
static void build_motion_hot_mask_and_score(const MotionHotspotRoi *roiInfo, const MotionHotspotStats *stats, const float *roi, float *hot, uint8_t *bw, float *score)
{
    for (int cx = 0; cx < roiInfo->nCols; ++cx)
    {
        for (int ry = 0; ry < roiInfo->nRows; ++ry)
        {
            int idx = ry * roiInfo->nCols + cx;
            if (roi[idx] > stats->T)
            {
                bw[idx] = 1;
                hot[idx] = roi[idx] - stats->T;
                score[cx] += hot[idx];
            }
        }
    }
}

// smooth the 1D motion score and compute its robust threshold.
static void smooth_motion_score_and_threshold(const MotionHotspotRoi *roiInfo, const float *score, float *scoreSmooth, MotionHotspotStats *stats)
{
    int halfSmooth = NEEDLE_MOTION_SCORE_SMOOTH_W / 2;
    for (int cx = 0; cx < roiInfo->nCols; ++cx)
    {
        int c0 = clampi(cx - halfSmooth, 0, roiInfo->nCols - 1);
        int c1 = clampi(cx + halfSmooth, 0, roiInfo->nCols - 1);
        float sum = 0.0f;
        int count = 0;
        for (int c = c0; c <= c1; ++c)
        {
            sum += score[c];
            count++;
        }
        scoreSmooth[cx] = (count > 0) ? sum / (float)count : score[cx];
    }

    float scoreTmp[NEEDLE_MOTION_MAX_ROI_COLS];
    float scoreDev[NEEDLE_MOTION_MAX_ROI_COLS];
    for (int cx = 0; cx < roiInfo->nCols; ++cx)
        scoreTmp[cx] = scoreSmooth[cx];

    float scoreMed = median_float(scoreTmp, roiInfo->nCols);
    for (int cx = 0; cx < roiInfo->nCols; ++cx)
        scoreDev[cx] = fabsf(scoreSmooth[cx] - scoreMed);

    stats->scoreSigma = fmaxf(1.4826f * median_float(scoreDev, roiInfo->nCols), 1.0e-3f);
    stats->scoreThr = scoreMed + NEEDLE_MOTION_SCORE_KSIGMA * stats->scoreSigma;
}

// flood-fill (grouping, similar to Matlab bwconncomp/regionprops functions) bright connected components and keep only compact, bright blobs.
static int find_motion_hotspot_blobs(const MotionHotspotRoi *roiInfo, const MotionHotspotStats *stats, const float *roi, const float *hot, const uint8_t *bw, uint8_t *visited, int *queue, MotionBlob *blobs)
{
    int numBlobs = 0;

    for (int idx0 = 0; idx0 < roiInfo->nPix; ++idx0)
    {
        if (!bw[idx0] || visited[idx0])
            continue;

        int qHead = 0;
        int qTail = 0;
        queue[qTail++] = idx0;
        visited[idx0] = 1;

        int area = 0;
        int minR = roiInfo->nRows, maxR = -1, minC = roiInfo->nCols, maxC = -1;
        float peak = -1.0e30f;
        float hotSum = 0.0f;
        float hotWeightedX = 0.0f;

        while (qHead < qTail)
        {
            int idx = queue[qHead++];
            int r = idx / roiInfo->nCols;
            int c = idx - r * roiInfo->nCols;
            area++;
            if (r < minR)
                minR = r;
            if (r > maxR)
                maxR = r;
            if (c < minC)
                minC = c;
            if (c > maxC)
                maxC = c;
            if (roi[idx] > peak)
                peak = roi[idx];

            hotSum += hot[idx];
            hotWeightedX += (float)(roiInfo->xA + c) * hot[idx];

            for (int dr = -1; dr <= 1; ++dr)
            {
                for (int dc = -1; dc <= 1; ++dc)
                {
                    if (dr == 0 && dc == 0)
                        continue;
                    int rr = r + dr;
                    int cc = c + dc;
                    if (rr < 0 || rr >= roiInfo->nRows || cc < 0 || cc >= roiInfo->nCols)
                        continue;
                    int ni = rr * roiInfo->nCols + cc;
                    if (bw[ni] && !visited[ni])
                    {
                        visited[ni] = 1;
                        queue[qTail++] = ni;
                    }
                }
            }
        }

        if (area < NEEDLE_MOTION_BLOB_MIN_AREA)
            continue;

        int width = maxC - minC + 1;
        int height = maxR - minR + 1;
        if (width < NEEDLE_MOTION_BLOB_MIN_WIDTH || height < NEEDLE_MOTION_BLOB_MIN_HEIGHT || width > NEEDLE_MOTION_BLOB_MAX_WIDTH || height > NEEDLE_MOTION_BLOB_MAX_HEIGHT)
            continue;

        float peakSigma = (peak - stats->bg) / stats->sigmaRob;
        float peakAboveThresholdSigma = (peak - stats->T) / stats->sigmaRob;
        float meanAboveThresholdSigma = (hotSum / (float)area) / stats->sigmaRob;
        float sumAboveThresholdSigma = hotSum / stats->sigmaRob;

        if (peakSigma < NEEDLE_MOTION_BLOB_MIN_PEAK_SIGMA || peakAboveThresholdSigma < NEEDLE_MOTION_BLOB_MIN_PEAK_ABOVE_THR_SIGMA ||
            meanAboveThresholdSigma < NEEDLE_MOTION_BLOB_MIN_MEAN_ABOVE_THR_SIGMA || sumAboveThresholdSigma < NEEDLE_MOTION_BLOB_MIN_SUM_ABOVE_THR_SIGMA)
            continue;

        if (numBlobs < NEEDLE_MOTION_MAX_BLOBS)
        {
            float compactnessPenalty = 0.15f * fmaxf((float)width / fmaxf((float)height, 1.0f), (float)height / fmaxf((float)width, 1.0f));
            blobs[numBlobs].area = area;
            blobs[numBlobs].width = width;
            blobs[numBlobs].height = height;
            blobs[numBlobs].centroidX = (hotSum > 0.0f) ? hotWeightedX / hotSum : (float)(roiInfo->xA + (minC + maxC) / 2);
            blobs[numBlobs].peakSigma = peakSigma;
            blobs[numBlobs].peakAboveThresholdSigma = peakAboveThresholdSigma;
            blobs[numBlobs].meanAboveThresholdSigma = meanAboveThresholdSigma;
            blobs[numBlobs].sumAboveThresholdSigma = sumAboveThresholdSigma;
            blobs[numBlobs].score = sumAboveThresholdSigma + 2.0f * peakAboveThresholdSigma + 0.25f * (float)area - compactnessPenalty;
            blobs[numBlobs].selectScore = -1.0e30f;
            numBlobs++;
        }
    }

    return numBlobs;
}

// pick the blob that is bright and closest to the temporal/profile expectation.
static int select_motion_hotspot_blob(const MotionHotspotRoi *roiInfo, MotionBlob *blobs, int numBlobs, int tipOriginal)
{
    int bestBlob = -1;
    float bestSelectScore = -1.0e30f;

    for (int bi = 0; bi < numBlobs; ++bi)
    {
        float distPrediction = fabsf(blobs[bi].centroidX - roiInfo->predictionX);
        float distProfile = fabsf(blobs[bi].centroidX - (float)tipOriginal);

        if (needle_tip_motion_state.hasMotionTrack)
        {
            if (distPrediction > NEEDLE_MOTION_TRACK_MAX_JUMP_PX)
                continue;
        }
        else if (distProfile > NEEDLE_MOTION_TRACK_ACQUIRE_MAX_DIST_PX)
            continue;

        blobs[bi].selectScore = blobs[bi].score - NEEDLE_MOTION_TRACK_DISTANCE_PENALTY * distPrediction;
        if (blobs[bi].selectScore > bestSelectScore)
        {
            bestSelectScore = blobs[bi].selectScore;
            bestBlob = bi;
        }
    }

    return bestBlob;
}

// apply temporal smoothing to the selected hotspot x-position.
static void smooth_motion_tip_from_blob(const MotionBlob *blob, float *tipMotion, float *newVelocity, int *newTrackCount)
{
    float rawMotionX = blob->centroidX;
    *tipMotion = rawMotionX;
    *newVelocity = 0.0f;
    *newTrackCount = needle_tip_motion_state.motionTrackCount + 1;

    if (needle_tip_motion_state.hasMotionTrack)
    {
        float rawStep = rawMotionX - needle_tip_motion_state.motionTrackX;
        rawStep = clampf(rawStep, -NEEDLE_MOTION_TRACK_MAX_JUMP_PX, NEEDLE_MOTION_TRACK_MAX_JUMP_PX);
        *tipMotion = needle_tip_motion_state.motionTrackX + NEEDLE_MOTION_TRACK_ALPHA * rawStep;
        *newVelocity = (1.0f - NEEDLE_MOTION_TRACK_VELOCITY_ALPHA) * needle_tip_motion_state.motionTrackV + NEEDLE_MOTION_TRACK_VELOCITY_ALPHA * (*tipMotion - needle_tip_motion_state.motionTrackX);
    }
}

// final safety gate: selected hotspot must have strong local score and reasonable correction size.
static bool validate_motion_tip_correction(const MotionHotspotRoi *roiInfo, const MotionHotspotStats *stats, const float *roi, const float *scoreSmooth,
                                           int tipOriginal, float tipMotion, float newVelocity, int newTrackCount, float *tipOut)
{
    int bestCol = clampi((int)floorf(tipMotion + 0.5f) - roiInfo->xA, 0, roiInfo->nCols - 1);
    float scoreAtMotion = scoreSmooth[bestCol];
    
    float confidence = fmaxf(0.0f, (scoreAtMotion - stats->scoreThr) / stats->scoreSigma);

    int c0 = clampi(bestCol - NEEDLE_MOTION_CENTROID_HALF_WINDOW_PX, 0, roiInfo->nCols - 1);
    int c1 = clampi(bestCol + NEEDLE_MOTION_CENTROID_HALF_WINDOW_PX, 0, roiInfo->nCols - 1);
    float localPeak = -1.0e30f;

    for (int r = 0; r < roiInfo->nRows; ++r)
    {
        for (int c = c0; c <= c1; ++c)
        {
            int idx = r * roiInfo->nCols + c;
            if (roi[idx] > localPeak)
                localPeak = roi[idx];
        }
    }

    float localPeakSigma = (localPeak - stats->bg) / stats->sigmaRob;
    float delta = tipMotion - (float)tipOriginal;
    bool hasHotSpot = (scoreAtMotion > stats->scoreThr) && (confidence >= NEEDLE_MOTION_MIN_SCORE_CONFIDENCE) && (localPeakSigma >= NEEDLE_MOTION_MIN_LOCAL_PEAK_SIGMA);

    if (!hasHotSpot || fabsf(delta) > NEEDLE_MOTION_MAX_CORRECTION_PX)
    {
        mark_motion_track_miss();
        return false;
    }

    update_motion_track_hit(tipMotion, newVelocity, newTrackCount);

    //if (fabsf(delta) < NEEDLE_MOTION_MIN_CORRECTION_PX || newTrackCount < NEEDLE_MOTION_TRACK_MIN_CONFIRM_FRAMES)
    //    return false;

    *tipOut = tipMotion;
    return true;
}

// locate a compact bright hotspot near the profile tip and return a motion-corrected tip.
static bool motion_hotspot_tip_from_diff(const float *Dorig, int Horig, int Worig, int Hrot, int Wrot, float theta_deg, int rho, int tipOriginal, int entryX, float *tipOut)
{
    static float roiRaw[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static float roi[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static float hot[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static float roiVals[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static float devVals[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static uint8_t bw[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static uint8_t visited[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static int queue[NEEDLE_MOTION_MAX_ROI_PIXELS];
    static float score[NEEDLE_MOTION_MAX_ROI_COLS];
    static float scoreSmooth[NEEDLE_MOTION_MAX_ROI_COLS];
    static MotionBlob blobs[NEEDLE_MOTION_MAX_BLOBS];

    rho = clampi(rho, 0, Hrot - 1);
    tipOriginal = clampi(tipOriginal, 0, Wrot - 1);
    entryX = clampi(entryX, 0, Wrot - 1);

    MotionHotspotRoi roiInfo;
    if (!setup_motion_hotspot_roi(Horig, Worig, Hrot, Wrot, theta_deg, rho, tipOriginal, entryX, &roiInfo))
    {
        mark_motion_track_miss();
        return false;
    }

    sample_motion_roi_from_diff(Dorig, Horig, Worig, &roiInfo, roiRaw);
    smooth_motion_roi_and_clear_scratch(&roiInfo, roiRaw, roi, roiVals, hot, bw, visited, score);

    MotionHotspotStats stats;
    if (!estimate_motion_hotspot_threshold(&roiInfo, roi, roiVals, devVals, &stats))
    {
        mark_motion_track_miss();
        return false;
    }

    build_motion_hot_mask_and_score(&roiInfo, &stats, roi, hot, bw, score);
    smooth_motion_score_and_threshold(&roiInfo, score, scoreSmooth, &stats);

    float maxScoreSmooth = scoreSmooth[0];
    for (int cx = 1; cx < roiInfo.nCols; ++cx)
    {
        if (scoreSmooth[cx] > maxScoreSmooth)
            maxScoreSmooth = scoreSmooth[cx];
    }
    if (maxScoreSmooth <= stats.scoreThr)
    {
        mark_motion_track_miss();
        return false;
    }

    int numBlobs = find_motion_hotspot_blobs(&roiInfo, &stats, roi, hot, bw, visited, queue, blobs);
    int bestBlob = select_motion_hotspot_blob(&roiInfo, blobs, numBlobs, tipOriginal);
    if (bestBlob < 0)
    {
        mark_motion_track_miss();
        return false;
    }

    float tipMotion = 0.0f;
    float newVelocity = 0.0f;
    int newTrackCount = 0;
    smooth_motion_tip_from_blob(&blobs[bestBlob], &tipMotion, &newVelocity, &newTrackCount);

    return validate_motion_tip_correction(&roiInfo, &stats, roi, scoreSmooth, tipOriginal, tipMotion, newVelocity, newTrackCount, tipOut);
}

// PIP core (needle detection)
static bool cached_line_geometry_is_usable(const NeedleLineResult *line, unsigned int Hout, unsigned int Wout)
{
    if (line == NULL)
        return false;
    if (Hout == 0 || Wout == 0 || Hout > MAX_SAMPLE_SIZE_NEEDLE || Wout > MAX_SAMPLE_SIZE_NEEDLE)
        return false;
    if (line->best_rho < 0 || line->best_rho >= (int)Hout)
        return false;
    if (line->tip_x < 0 || line->tip_x >= (int)Wout ||
        line->entry_x < 0 || line->entry_x >= (int)Wout)
        return false;

    int len = abs(line->entry_x - line->tip_x) + 1;
    return (len >= MIN_VALID_NEEDLE_SIZE_PIXELS && len <= MAX_NEEDLE_SIZE_PIXELS);
}

// return a percentile from an already sorted cached-line sample array.
static float cached_line_percentile_sorted(const float *sortedVals, int len, int pct)
{
    if (len <= 0)
        return 0.0f;

    int idx = ((len - 1) * pct + 50) / 100;
    if (idx < 0) idx = 0;
    if (idx >= len) idx = len - 1;
    return sortedVals[idx];
}

// fill small 1D gaps in the cached-line confidence hit mask.
static void cached_line_fill_small_gaps(const uint8_t *maskIn, int len, int maxGap, uint8_t *maskOut)
{
    memcpy(maskOut, maskIn, (size_t)len * sizeof(uint8_t));

    int i = 0;
    while (i < len)
    {
        if (maskOut[i])
        {
            ++i;
            continue;
        }

        int start = i;
        while (i < len && !maskOut[i])
            ++i;
        int end = i - 1;
        int gapLen = end - start + 1;
        bool touchesEdge = (start == 0 || end == len - 1);

        if (!touchesEdge && gapLen <= maxGap)
        {
            for (int k = start; k <= end; ++k)
                maskOut[k] = 1U;
        }
    }
}

// count the longest contiguous support run along a cached line.
static int cached_line_longest_run(const uint8_t *mask, int len)
{
    int best = 0;
    int i = 0;

    while (i < len)
    {
        while (i < len && !mask[i])
            ++i;
        int start = i;
        while (i < len && mask[i])
            ++i;
        int runLen = i - start;
        if (runLen > best)
            best = runLen;
    }

    return best;
}

// sample current-frame intensity near a cached line point and average the top three pixels.
static float cached_line_band_top3_mean(const uint8_t *I, int H, int W, float x, float y,
                                        float nx, float ny, int bandR)
{
    float top1 = -1.0f;
    float top2 = -1.0f;
    float top3 = -1.0f;
    int count = 0;

    for (int off = -bandR; off <= bandR; ++off)
    {
        int sx = (int)floorf(x + nx * (float)off + 0.5f);
        int sy = (int)floorf(y + ny * (float)off + 0.5f);
        if (sx < 0 || sx >= W || sy < 0 || sy >= H)
            continue;

        float v = (float)I[(size_t)sx * (size_t)H + (size_t)sy];
        ++count;
        if (v >= top1)
        {
            top3 = top2;
            top2 = top1;
            top1 = v;
        }
        else if (v >= top2)
        {
            top3 = top2;
            top2 = v;
        }
        else if (v > top3)
        {
            top3 = v;
        }
    }

    if (count <= 0)
        return 0.0f;
    if (count == 1)
        return top1;
    if (count == 2)
        return 0.5f * (top1 + top2);
    return (top1 + top2 + top3) / 3.0f;
}

// compute cheap per-physical-frame confidence along the cached virtual line.
static float cached_line_confidence_current_frame(const uint8_t *I, unsigned int H, unsigned int W, const NeedleLineResult *line, unsigned int Hout, unsigned int Wout)
{
    if (!cached_line_geometry_is_usable(line, Hout, Wout))
        return 0.0f;

    const int H_i = (int)H;
    const int W_i = (int)W;
    int len = abs(line->entry_x - line->tip_x) + 1;
    if (len < MIN_VALID_NEEDLE_SIZE_PIXELS || len > MAX_NEEDLE_SIZE_PIXELS)
        return 0.0f;

    const float theta_deg = line->theta_deg;
    const float rad  = theta_deg * (float)M_PI / 180.0f;
    const float cosA = cosf(rad);
    const float sinA = sinf(rad);

    const float cx_in  = ((float)W - 1.0f) * 0.5f;
    const float cy_in  = ((float)H - 1.0f) * 0.5f;
    const float cx_out = ((float)Wout - 1.0f) * 0.5f;
    const float cy_out = ((float)Hout - 1.0f) * 0.5f;

    const float tipXr = (float)line->tip_x - cx_out;
    const float tipYr = (float)line->best_rho - cy_out;
    const float entryXr = (float)line->entry_x - cx_out;
    const float entryYr = (float)line->best_rho - cy_out;

    const float tipX =  cosA * tipXr + sinA * tipYr + cx_in;
    const float tipY = -sinA * tipXr + cosA * tipYr + cy_in;
    const float entryX =  cosA * entryXr + sinA * entryYr + cx_in;
    const float entryY = -sinA * entryXr + cosA * entryYr + cy_in;

    float segDx = entryX - tipX;
    float segDy = entryY - tipY;
    float segLen = sqrtf(segDx * segDx + segDy * segDy);
    if (segLen < 1.0e-3f)
        return 0.0f;

    float nx = -segDy / segLen;
    float ny =  segDx / segLen;

    int bandR = BAND_R;
    if (bandR <= 0)
        bandR = 1;

    for (int i = 0; i < len; ++i)
    {
        float t = (len <= 1) ? 0.0f : (float)i / (float)(len - 1);
        float x = tipX + t * segDx;
        float y = tipY + t * segDy;
        cached_line_conf_vals[i] = cached_line_band_top3_mean(I, H_i, W_i, x, y, nx, ny, bandR);
        cached_line_conf_sorted[i] = cached_line_conf_vals[i];
    }

    for (int i = 1; i < len; ++i)
    {
        float key = cached_line_conf_sorted[i];
        int j = i - 1;
        while (j >= 0 && cached_line_conf_sorted[j] > key)
        {
            cached_line_conf_sorted[j + 1] = cached_line_conf_sorted[j];
            --j;
        }
        cached_line_conf_sorted[j + 1] = key;
    }

    float p20 = cached_line_percentile_sorted(cached_line_conf_sorted, len, 20);
    float p90 = cached_line_percentile_sorted(cached_line_conf_sorted, len, 90);
    float span = p90 - p20;
    if (span < 1.0f)
        span = 1.0f;

    float eta = span / fmaxf(p90, 1.0f);
    float thrLow = p20 + 0.20f * eta * span;
    float thrHigh = p20 + 0.50f * eta * span;
    float tSat = p20 + 0.85f * eta * span;

    if (thrLow < (float)NEEDLE_SHAFT_INTENSITY_P20_MIN)
        thrLow = (float)NEEDLE_SHAFT_INTENSITY_P20_MIN;
    if (thrHigh < (float)NEEDLE_SHAFT_INTENSITY_P50_MIN)
        thrHigh = (float)NEEDLE_SHAFT_INTENSITY_P50_MIN;
    if (tSat < (float)NEEDLE_SHAFT_INTENSITY_P85_MIN)
        tSat = (float)NEEDLE_SHAFT_INTENSITY_P85_MIN;
    if (thrLow > 250.0f)
        thrLow = 250.0f;
    if (thrHigh > 252.0f)
        thrHigh = 252.0f;
    if (tSat > 255.0f)
        tSat = 255.0f;
    if (thrHigh <= thrLow)
        thrHigh = thrLow + 1.0f;
    if (tSat <= thrHigh)
        tSat = thrHigh + 1.0f;

    memset(cached_line_conf_mask, 0, (size_t)len * sizeof(uint8_t));
    int nHit = 0;
    int i = 0;
    while (i < len)
    {
        if (cached_line_conf_vals[i] < thrLow)
        {
            ++i;
            continue;
        }

        int start = i;
        bool hasStrong = false;
        while (i < len && cached_line_conf_vals[i] >= thrLow)
        {
            if (cached_line_conf_vals[i] >= thrHigh)
                hasStrong = true;
            ++i;
        }

        if (hasStrong)
        {
            for (int k = start; k < i; ++k)
                cached_line_conf_mask[k] = 1U;
            nHit += i - start;
        }
    }

    float support = (float)nHit / (float)len;
    cached_line_fill_small_gaps(cached_line_conf_mask, len, 1, cached_line_conf_filled);
    int longestRun = cached_line_longest_run(cached_line_conf_filled, len);
    float continuity = 0.7f * ((float)longestRun / (float)len) + 0.3f * support;

    float den = tSat - thrHigh;
    if (den < 1.0f)
        den = 1.0f;

    float sumNorm = 0.0f;
    for (int k = 0; k < len; ++k)
    {
        if (cached_line_conf_vals[k] <= thrHigh)
            continue;
        if (cached_line_conf_vals[k] >= tSat)
            sumNorm += 1.0f;
        else
            sumNorm += (cached_line_conf_vals[k] - thrHigh) / den;
    }

    float strength = sumNorm / (float)len;
    float centerEnergy = support * strength;
    if (centerEnergy <= 0.0f)
        return 0.0f;

    float confBaseSum = support + strength + continuity;
    if (confBaseSum < CONF_BASE_THRESH)
        return confBaseSum;

    float confidenceBase = 0.33f * support + 0.34f * strength + 0.33f * continuity;
    return clampf(confidenceBase * 0.85f, 0.0f, 1.0f);
}

// main PIP detector: find line angle/rho, rescore top candidates (if turned on), pick tip, and update confidence.
static NeedleLineResult pip_with_prior_info(const uint8_t *I_in, unsigned int H, unsigned int W, int needle_len_px, const NeedlePipParams *params_in,
                                            unsigned int outN, unsigned int *Hout, unsigned int *Wout, const DepthMaskParams *depth_params_in)
{
    if(Hout) 
        *Hout = 0; 
    if(Wout) 
        *Wout = 0;

    NeedleLineResult res;
    res.best_rho = 0;
    res.theta_deg = 0.0;
    res.entry_x = res.entry_y = res.tip_x = res.tip_y = 0;
    res.detected = false;
    res.conf_score = 0.0;

    if (((size_t)H) * ((size_t)W) > MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE || outN > MAX_SAMPLE_SIZE_NEEDLE)
    {
        LogDebug("pip_with_prior_info call: out of bound H = %u, W = %u, outN = %u\n", H, W, outN);
        //exit(EXIT_FAILURE);
        return res;
    }

    float *rot_best = NULL; // points to the selected top-3 rotated-image buffer

    NeedlePipParams p = {
        .theta_step_deg      = 1.0f,
        .theta_range_min_deg = 0.0f,
        .theta_range_max_deg = 179.0f,
        .resize_factor       = 1.0f,
        .normalize           = false
    };
    if (params_in)
        p = *params_in;

    unsigned int Npix = H * W;

    memcpy(I_proc, I_in, Npix * sizeof(uint8_t));
    
    //run depth mask to mask the skin layer and bottom lines of the frame image
    DepthMaskParams d = depthMaskParams;
    if (depth_params_in) d = *depth_params_in;
    depth_mask(H, W, needle_len_px, &d, I_proc);
    //end depth_mask

#if NEEDLE_ENABLE_MOTION_TIP_CORRECTION
    update_motion_difference_image(I_proc, H, W);
#endif

    int y;
    float theta;
    
    // best line parameters
    float bestSum  = -1.0e30;
    int best_rho = 0;
    float best_theta = 0.0;
    unsigned int best_Ht = 0, best_Wt = 0;
    bool best_line_update = false;
    NeedleLineCandidate topCandidates[NEEDLE_TOP_LINE_CANDIDATE_COUNT];
    init_line_candidates(topCandidates);

    //int needle_len_rot = (int)(needle_len_px * p.resize_factor + 0.5f);
    unsigned int needle_len_rot = (unsigned int)needle_len_px;
    if (needle_len_rot < 1)
        needle_len_rot = 1;
    if (needle_len_rot > outN)
        needle_len_rot = outN;

    float theta_range_min_deg = p.theta_range_min_deg;
    float theta_range_max_deg = p.theta_range_max_deg;
    if(NEEDLE_INSERSION_SIDE_RIGHT == 0) // needle is inserted from left of the image
    {
        theta_range_min_deg = 180.0 - p.theta_range_max_deg;
        theta_range_max_deg = 180.0 - p.theta_range_min_deg;
    }

    for (theta = theta_range_min_deg; theta <= theta_range_max_deg + 0.5f; theta += p.theta_step_deg)
    {
        best_line_update = false;
        bounding_box(H, W, theta, Hout, Wout);

        // bounding box doesn't fit in allocated outN x outN buffers, skip this angle safely.
        if (*Wout > outN || *Hout > outN || *Hout < 1 || *Wout < 1)
            continue;

        accumulate_rotated_profile_forward(I_proc, H, W, theta, *Hout, *Wout, needle_len_rot, profile);

        // Find best row for this angle
        for (y = 0; y < (int)*Hout; ++y)
        {
            float v = profile[y];
            if (v > bestSum)
            {
                bestSum = v;
                best_rho = y;
                best_line_update = true;
                best_Ht = *Hout;
                best_Wt = *Wout;
            }
        }
        if (best_line_update)
        {
            best_theta = theta;
        }

        int selectedRows[NEEDLE_TOP_LINE_CANDIDATE_ROWS_PER_ANGLE];
        for (int ci = 0; ci < NEEDLE_TOP_LINE_CANDIDATE_ROWS_PER_ANGLE; ++ci)
            selectedRows[ci] = -1;

        int nTopRows = NEEDLE_TOP_LINE_CANDIDATE_ROWS_PER_ANGLE;
        if (nTopRows > (int)*Hout)
            nTopRows = (int)*Hout;

        for (int ci = 0; ci < nTopRows; ++ci)
        {
            float localBest = -1.0e30f;
            int localBestRow = -1;

            for (y = 0; y < (int)*Hout; ++y)
            {
                bool alreadySelected = false;
                for (int sj = 0; sj < ci; ++sj)
                {
                    if (selectedRows[sj] == y)
                    {
                        alreadySelected = true;
                        break;
                    }
                }
                if (!alreadySelected && profile[y] > localBest)
                {
                    localBest = profile[y];
                    localBestRow = y;
                }
            }

            if (localBestRow >= 0 && localBest > 0.0f)
            {
                selectedRows[ci] = localBestRow;

                NeedleLineCandidate cand;
                memset(&cand, 0, sizeof(cand));
                cand.valid = true;
                cand.scoreSum = localBest;
                cand.theta_deg = theta;
                cand.rho = localBestRow;
                cand.Ht = *Hout;
                cand.Wt = *Wout;
                cand.tip_x = 0;
                cand.entry_x = (NEEDLE_INSERSION_SIDE_RIGHT && row_seg_right[localBestRow] >= row_seg_left[localBestRow]) ?
                    row_seg_right[localBestRow] : row_seg_left[localBestRow];
                cand.segmentMean = -1.0e30f;
                cand.rotBufferIdx = -1;
                insert_top_line_candidate(topCandidates, cand);
            }
        }
    }

    NeedleLineCandidate selectedCand = {0};
    selectedCand.scoreSum = -1.0e30f;
    selectedCand.segmentMean = -1.0e30f;
    float bestSegmentMean = -1.0e30f;

    for (int ci = 0; ci < NEEDLE_TOP_LINE_CANDIDATE_COUNT; ++ci)
    {
        if (!topCandidates[ci].valid)
            continue;
        if (topCandidates[ci].Ht < 1 || topCandidates[ci].Wt < 1 || topCandidates[ci].Ht > outN || topCandidates[ci].Wt > outN)
            continue;

        float cornerX[4];
        float cornerY[4];
        rotated_original_frame_corners(H, W, topCandidates[ci].Ht, topCandidates[ci].Wt, topCandidates[ci].theta_deg, cornerX, cornerY);

        NeedleLineCandidate evalCand;
        memset(&evalCand, 0, sizeof(evalCand));
        evalCand.segmentMean = -1.0e30f;
        if (evaluate_line_candidate_by_direct_sampling(I_proc, H, W, topCandidates[ci].Ht, topCandidates[ci].Wt, topCandidates[ci].theta_deg, cornerX, cornerY, topCandidates[ci].rho, topCandidates[ci].scoreSum, &evalCand))
        {
            if (evalCand.segmentMean > bestSegmentMean)
            {
                bestSegmentMean = evalCand.segmentMean;
                selectedCand = evalCand;
                selectedCand.valid = true;
            }
        }
    }

    if (!selectedCand.valid && best_Ht >= 1 && best_Wt >= 1 && best_Ht <= outN && best_Wt <= outN)
    {
        selectedCand.valid = true;
        selectedCand.scoreSum = bestSum;
        selectedCand.theta_deg = best_theta;
        selectedCand.rho = best_rho;
        selectedCand.Ht = best_Ht;
        selectedCand.Wt = best_Wt;
        selectedCand.rotBufferIdx = 0;

        int x_left_bound = 0;
        int x_right_bound = 0;
        if (rotated_row_original_x_bounds(H, W, selectedCand.Ht, selectedCand.Wt, selectedCand.theta_deg, selectedCand.rho, &x_left_bound, &x_right_bound))
            selectedCand.entry_x = NEEDLE_INSERSION_SIDE_RIGHT ? x_right_bound : x_left_bound;
        else
            selectedCand.entry_x = NEEDLE_INSERSION_SIDE_RIGHT ? (int)selectedCand.Wt - 1 : 0;

        DirectRotSampler fallbackSampler;
        init_direct_rot_sampler(&fallbackSampler, I_proc, H, W, selectedCand.Ht, selectedCand.Wt, selectedCand.theta_deg);
        selectedCand.tip_x = find_tip_x_along_best_line_direct(&fallbackSampler, selectedCand.rho);
    }

    if (selectedCand.valid)
    {
        selectedCand.rotBufferIdx = 0;
        rot_best = top_line_rot_buffer(0);
        rotate_image_bilinear(I_proc, H, W, selectedCand.theta_deg, rot_best, outN, selectedCand.Ht, selectedCand.Wt);
    }

    best_rho = selectedCand.rho;
    best_theta = selectedCand.theta_deg;
    best_Ht = selectedCand.Ht;
    best_Wt = selectedCand.Wt;

    // rotated processed image
    // bounding box doesn't fit in allocated outN x outN buffers, skip this rotation angle safely.
    if(best_Ht < 1 || best_Wt < 1 || best_Ht > outN || best_Wt > outN)
    {
        LogDebug("pip_with_prior_info call: invalid output size Hout = %u, Wout = %u\n", best_Ht, best_Wt);
        return res;
    }

    *Wout = best_Wt;
    *Hout = best_Ht;

    int entry_x, entry_y, tip_x, tip_y;
    entry_y = tip_y = best_rho;
    entry_x   = selectedCand.entry_x;

    tip_x = selectedCand.tip_x;
    if (tip_x <= 0)
        tip_x = find_tip_x_along_best_line(rot_best, best_Ht, best_Wt, outN, best_rho);

    res.best_rho  = best_rho;
    res.theta_deg = best_theta;
    res.entry_x   = entry_x;
    res.entry_y   = entry_y;
    res.tip_x     = tip_x;
    res.tip_y     = tip_y;
    
#if NEEDLE_ENABLE_FINAL_LINE_REFINEMENT
    refine_detected_needle_line(rot_best, best_Ht, best_Wt, outN, &res);
#endif

#if NEEDLE_REJECT_ENTRY_TOO_DEEP
    float entryOrigY = 0.0f;
    float entryLimitY = 0.0f;
    if (entry_is_too_deep(H, W, best_Ht, best_Wt, best_theta, res.entry_x, res.entry_y, &entryOrigY, &entryLimitY))
    {
        LogDebug("Needle line rejected: entry too deep. entryY=%.2f limitY=%.2f theta=%.2f rho=%d\n", entryOrigY, entryLimitY, best_theta, best_rho);
        reset_tip_motion_state();
        res.detected = false;
        return res;
    }
#endif

#if NEEDLE_ENABLE_MOTION_TIP_CORRECTION
    if (needle_tip_motion_state.hasLine)
    {
        float thetaJump = angle_difference_deg(best_theta, needle_tip_motion_state.thetaDeg);
        int rhoJump = abs(best_rho - needle_tip_motion_state.rho);
        if (thetaJump > NEEDLE_MOTION_TRACK_LINE_THETA_JUMP_DEG || rhoJump > NEEDLE_MOTION_TRACK_LINE_RHO_JUMP_PX)
            reset_tip_motion_state();
    }

    float tipMotion = (float)res.tip_x;
    bool usedMotionCorrection = false;

    if (motion_hotspot_tip_from_diff(motion_diff, H, W, best_Ht, best_Wt, best_theta, best_rho, res.tip_x, res.entry_x, &tipMotion))
    {
        res.tip_x = clampi((int)floorf(tipMotion + 0.5f), 0, (int)best_Wt - 1);
        usedMotionCorrection = true;
    }

    needle_tip_motion_state.hasTip = true;
    needle_tip_motion_state.tipX = (float)res.tip_x;
    needle_tip_motion_state.hasLine = true;
    needle_tip_motion_state.thetaDeg = best_theta;
    needle_tip_motion_state.rho = best_rho;
#endif

    if(!needle_confidence_on)
    {
        res.detected = true;
        return res;
    }

    // calculate the confidence score and determine if the detection is valid
    NeedleConfidenceSingleFrame conf_single_frame = needle_line_confidence_singleFrame(rot_best, best_Ht, best_Wt, outN, &res);
    latest_virtual_conf_frame = conf_single_frame.confidence_score_singleFrame;
    latest_virtual_conf_frame_valid = true;
    needle_line_confidence_multiFrames(conf_single_frame.confidence_score_singleFrame, res);

    res.conf_score = needleMultiFrameState.confSmooth;
    
    // refine detected line and tip location with multi-frame EMA smoothing
   /*res.tip_x = (int)needleMultiFrameState.smoothTip_x;
    res.tip_y = (int)needleMultiFrameState.smoothRho;
    res.entry_y = (int)needleMultiFrameState.smoothRho;
    res.best_rho = (int)needleMultiFrameState.smoothRho;
    res.theta_deg = needleMultiFrameState.smoothTheta_deg;*/

    /*LogInfo("//CLI PIP confidence: conf_single_frame = %.3f, conf_multiFrames = %.3f, tracking = %d, enhanceOn = %d, enhanceActive = %d, reverb_ratio = %.3f, reverb_score = %.3f, support = %.3f, %.3f, %.3f\n", 
        conf_single_frame.confidence_score_singleFrame, res.conf_score, needleMultiFrameState.tracked, needleMultiFrameState.enhancementLogicOn, needleMultiFrameState.enhancementActive, conf_single_frame.centerEv.reverb_ratio, conf_single_frame.centerEv.reverb_score, conf_single_frame.centerEv.support,
        conf_single_frame.centerEv.strength, conf_single_frame.centerEv.continuity);*/
    /*LogInfo(" %.3f %.3f %d %d %d %.3f %.3f %.3f %.3f %.3f\n", 
        conf_single_frame.confidence_score_singleFrame, res.conf_score, needleMultiFrameState.tracked, needleMultiFrameState.enhancementLogicOn, needleMultiFrameState.enhancementActive, conf_single_frame.centerEv.reverb_ratio, conf_single_frame.centerEv.reverb_score, conf_single_frame.centerEv.support,
        conf_single_frame.centerEv.strength, conf_single_frame.centerEv.continuity);*/
    
    
    if(needleMultiFrameState.enhancementActive)
        res.detected  = true;

    return res;
}

// erosion then dilation with disk radius 2
void morphologicopen_disk2(uint8_t *in, uint8_t *out, int H, int W)
{
    if (H < 5 || W < 5)
    { 
        memcpy(out, in, H * W); 
        return;
    } 

    int offsets[13];
    
    // precompute neighbor offsets to avoid multiplication inside loop
    for (int k = 0; k < 13; ++k)
    {
        offsets[k] = dx[k] * H + dy[k];
    }

    // initialize to 0 
    int N = W * H;
    memset(out, 0, N * sizeof(uint8_t));
    //memset(tmp, 0, N * sizeof(uint8_t));
    memset(tmp_morphologic, 0, N * sizeof(uint8_t));

    int x, y, k, idx;
    uint8_t v;
    // erosion
    // process only pixels where the full disk fits with disk radius of 2 pixels
    for ( x = 2; x < W - 2; ++x)
    {
        for (y = 2; y < H - 2; ++y)
        {
            idx = x * H + y;
            uint8_t minv = in[idx];

            // get the min value over all 13 adjacient points
            for (k = 0; k < 13; ++k)
            {
                v = in[idx + offsets[k]];
                if (v < minv)
                    minv = v;
            }

            //tmp[idx] = minv;
            tmp_morphologic[idx] = minv;
        }
    }

    // dilation
    for (x = 2; x < W - 2; ++x)
    {
        for (y = 2; y < H - 2; ++y)
        {
            idx = x * H + y;
           // uint8_t maxv = tmp[idx];
           uint8_t maxv = tmp_morphologic[idx];

            for (k = 0; k < 13; ++k)
            {
                //v = tmp[idx + offsets[k]];
                v = tmp_morphologic[idx + offsets[k]];
                if (v > maxv)
                    maxv = v;
            }
            out[idx] = maxv;
        }
    }
}

// read a uint8 pixel inside the cached fusion workbox with replicated image borders.
static inline uint8_t get_u8_work_replicate(const uint8_t *img, int H, int W, int x, int y, int x0, int x1, int y0, int y1)
{
    if (x < 0) x = 0;
    else if (x >= W) x = W - 1;
    if (y < 0) y = 0;
    else if (y >= H) y = H - 1;

    if (x < x0 || x > x1 || y < y0 || y > y1)
        return 0;
    return img[(size_t)x * (size_t)H + (size_t)y];
}

// read a uint16 pixel inside the cached fusion workbox with replicated image borders.
static inline uint16_t get_u16_work_replicate(const uint16_t *img, int H, int W, int x, int y, int x0, int x1, int y0, int y1)
{
    if (x < 0)
        x = 0;
    else if (x >= W)
        x = W - 1;
    if (y < 0)
        y = 0;
    else if (y >= H)
        y = H - 1;

    if (x < x0 || x > x1 || y < y0 || y > y1)
        return 0;
    return img[(size_t)x * (size_t)H + (size_t)y];
}

// read a uint8 pixel and return zero outside the cached fusion workbox.
static inline uint8_t get_u8_work_zero(const uint8_t *img, int H, int W, int x, int y, int x0, int x1, int y0, int y1)
{
    if (x < 0 || x >= W || y < 0 || y >= H)
        return 0;
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return 0;
    return img[(size_t)x * (size_t)H + (size_t)y];
}

// check whether the precomputed fusion workbox still matches the active detected line.
static bool fusion_geometry_cache_matches(unsigned int H, unsigned int W, const NeedleLineResult *line, unsigned int rows_rot, unsigned int cols_rot)
{
    return fusion_geometry_cache.valid && fusion_geometry_cache.H == H && fusion_geometry_cache.W == W && fusion_geometry_cache.rowsRot == rows_rot &&
           fusion_geometry_cache.colsRot == cols_rot && fusion_geometry_cache.line.best_rho == line->best_rho && fusion_geometry_cache.line.entry_x == line->entry_x &&
           fusion_geometry_cache.line.tip_x == line->tip_x && fusion_geometry_cache.line.theta_deg == line->theta_deg;
}

// precompute original-frame line mask, workbox, and morphology offsets for repeated fusion.
static bool build_fusion_geometry_cache(unsigned int H, unsigned int W, const NeedleLineResult *line, unsigned int rows_rot, unsigned int cols_rot)
{
    const int radius = 3;
    const int morphR = 2;
    const int workPad = NEEDLE_WIDTH + radius + 2 * morphR + 3;
    const int H_i = (int)H;
    const int W_i = (int)W;

    fusion_geometry_cache.valid = false;

    if (H == 0 || W == 0 || H > MAX_SAMPLE_SIZE_NEEDLE ||
        W > MAX_SCANLINES_NEEDLE || line == NULL)
        return false;

    const float theta_deg = line->theta_deg;
    const float rad  = theta_deg * (float)M_PI / 180.0f;
    const float cosA = cosf(rad);
    const float sinA = sinf(rad);

    const float cx_in  = ((float)W - 1.0f) * 0.5f;
    const float cy_in  = ((float)H - 1.0f) * 0.5f;
    const float cx_out = ((float)cols_rot - 1.0f) * 0.5f;
    const float cy_out = ((float)rows_rot - 1.0f) * 0.5f;

    const float x0r = (float)line->entry_x - cx_out;
    const float y0r = (float)line->best_rho - cy_out;
    const float x1r = (float)line->tip_x - cx_out;
    const float y1r = (float)line->best_rho - cy_out;

    const float x0 =  cosA * x0r + sinA * y0r + cx_in;
    const float y0 = -sinA * x0r + cosA * y0r + cy_in;
    const float x1 =  cosA * x1r + sinA * y1r + cx_in;
    const float y1 = -sinA * x1r + cosA * y1r + cy_in;

    float segDx = x1 - x0;
    float segDy = y1 - y0;
    float seg_length_squared = segDx * segDx + segDy * segDy;
    if (seg_length_squared < 1.0e-20f)
    {
        LogDebug("fusion geometry cache: segment too short, skip mask fusion\n");
        return false;
    }

    float minLineX = (x0 < x1) ? x0 : x1;
    float maxLineX = (x0 > x1) ? x0 : x1;
    float minLineY = (y0 < y1) ? y0 : y1;
    float maxLineY = (y0 > y1) ? y0 : y1;

    int workX0 = clampi((int)floorf(minLineX - (float)workPad), 0, W_i - 1);
    int workX1 = clampi((int)ceilf (maxLineX + (float)workPad), 0, W_i - 1);
    int workY0 = clampi((int)floorf(minLineY - (float)workPad), 0, H_i - 1);
    int workY1 = clampi((int)ceilf (maxLineY + (float)workPad), 0, H_i - 1);

    if (workX1 < workX0 || workY1 < workY0)
        return false;

    memset(fusion_line_mask, 0, (size_t)H * (size_t)W * sizeof(uint8_t));

    const float r = (float)NEEDLE_WIDTH;
    const float r2 = r * r;
    // Build a binary line mask once; skipped frames reuse it with current pixel values.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            float px = (float)x;
            float py = (float)y;
            float t = ((px - x0) * segDx + (py - y0) * segDy) / seg_length_squared;
            if (t < 0.0f)
                t = 0.0f;
            else if (t > 1.0f)
                t = 1.0f;

            float cx = x0 + t * segDx;
            float cy = y0 + t * segDy;
            float ex = px - cx;
            float ey = py - cy;
            float d2 = ex * ex + ey * ey;
            size_t idx = (size_t)x * (size_t)H + (size_t)y;

            fusion_line_mask[idx] = (d2 <= r2) ? 1U : 0U;
        }
    }

    fusion_geometry_cache.line = *line;
    fusion_geometry_cache.H = H;
    fusion_geometry_cache.W = W;
    fusion_geometry_cache.rowsRot = rows_rot;
    fusion_geometry_cache.colsRot = cols_rot;
    fusion_geometry_cache.workX0 = workX0;
    fusion_geometry_cache.workX1 = workX1;
    fusion_geometry_cache.workY0 = workY0;
    fusion_geometry_cache.workY1 = workY1;
    fusion_geometry_cache.workRows = workY1 - workY0 + 1;

    for (int k = 0; k < 13; ++k)
        fusion_geometry_cache.morphOffsets[k] = (int)dx[k] * H_i + (int)dy[k];

    fusion_geometry_cache.valid = true;
    return true;
}

// reuse cached fusion geometry or rebuild it when the detected line changes.
static bool ensure_fusion_geometry_cache(unsigned int H, unsigned int W, const NeedleLineResult *line, unsigned int rows_rot, unsigned int cols_rot)
{
    if (fusion_geometry_cache_matches(H, W, line, rows_rot, cols_rot))
        return true;

    return build_fusion_geometry_cache(H, W, line, rows_rot, cols_rot);
}

// fuse enhancement only inside the cached workbox around the detected needle line.
static int fuse_needle_mask_optimized(int frameNumber_cur, uint8_t *I_orig, unsigned int H, unsigned int W, const NeedleLineResult *line, unsigned int rows_rot,  unsigned int cols_rot)
{
    (void)frameNumber_cur;

    if (!ensure_fusion_geometry_cache(H, W, line, rows_rot, cols_rot))
        return 1;

    const int radius = 3;
    const int morphR = 2;
    const int H_i = (int)H;
    const int W_i = (int)W;
    const int workX0 = fusion_geometry_cache.workX0;
    const int workX1 = fusion_geometry_cache.workX1;
    const int workY0 = fusion_geometry_cache.workY0;
    const int workY1 = fusion_geometry_cache.workY1;
    const int workRows = fusion_geometry_cache.workRows;

    for (int x = workX0; x <= workX1; ++x)
    {
        memset(fused_MIP + (size_t)x * H + (size_t)workY0, 0, (size_t)workRows * sizeof(uint8_t));
        memset(tmp_fuse + (size_t)x * H + (size_t)workY0, 0, (size_t)workRows * sizeof(uint8_t));
        memset(tmp_morphologic + (size_t)x * H + (size_t)workY0, 0, (size_t)workRows * sizeof(uint8_t));
        memset(tmp16_fuse + (size_t)x * H + (size_t)workY0, 0, (size_t)workRows * sizeof(uint16_t));
    }

    // raw line mask uses precomputed geometry; only current pixel intensity changes per frame.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            size_t idx = (size_t)x * H + (size_t)y;
            fused_MIP[idx] = fusion_line_mask[idx] ? I_orig[idx] : 0;
        }
    }

    const int win = 2 * radius + 1;
    const int norm = win * win;

    // first pass of separable box filter: vertical accumulation into uint16 scratch.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            uint32_t sum = 0;
            for (int ky = -radius; ky <= radius; ++ky)
                sum += get_u8_work_replicate(fused_MIP, H_i, W_i, x, y + ky, workX0, workX1, workY0, workY1);
            tmp16_fuse[(size_t)x * H + (size_t)y] = (uint16_t)sum;
        }
    }

    // second pass of separable box filter: horizontal accumulation and normalization.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            uint32_t sum = 0;
            for (int kx = -radius; kx <= radius; ++kx)
                sum += get_u16_work_replicate(tmp16_fuse, H_i, W_i, x + kx, y, workX0, workX1, workY0, workY1);
            tmp_fuse[(size_t)x * H + (size_t)y] = (uint8_t)((sum + (norm / 2)) / norm);
        }
    }

    // erosion pass of the disk-2 morphology inside the same cached workbox.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            size_t idx = (size_t)x * H + (size_t)y;
            if (x < morphR || x >= W_i - morphR || y < morphR || y >= H_i - morphR)
            {
                tmp_morphologic[idx] = 0;
                continue;
            }

            uint8_t minv = get_u8_work_zero(tmp_fuse, H_i, W_i, x, y, workX0, workX1, workY0, workY1);
            for (int k = 0; k < 13; ++k)
            {
                int nx = x + dx[k];
                int ny = y + dy[k];
                uint8_t v = (nx < workX0 || nx > workX1 || ny < workY0 || ny > workY1) ?
                    0 : tmp_fuse[idx + fusion_geometry_cache.morphOffsets[k]];
                if (v < minv)
                    minv = v;
            }
            tmp_morphologic[idx] = minv;
        }
    }

    // erosion pass of the disk-2 morphology inside the same cached workbox.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            size_t idx = (size_t)x * H + (size_t)y;
            if (x < morphR || x >= W_i - morphR || y < morphR || y >= H_i - morphR)
            {
                fused_MIP[idx] = 0;
                continue;
            }

            uint8_t maxv = get_u8_work_zero(tmp_morphologic, H_i, W_i, x, y, workX0, workX1, workY0, workY1);
            for (int k = 0; k < 13; ++k)
            {
                int nx = x + dx[k];
                int ny = y + dy[k];
                uint8_t v = (nx < workX0 || nx > workX1 || ny < workY0 || ny > workY1) ? 0 : tmp_morphologic[idx + fusion_geometry_cache.morphOffsets[k]];
                if (v > maxv)
                    maxv = v;
            }
            fused_MIP[idx] = maxv;
        }
    }

    // final additive fusion is limited to the workbox, not the full ROI.
    for (int x = workX0; x <= workX1; ++x)
    {
        for (int y = workY0; y <= workY1; ++y)
        {
            size_t idx = (size_t)x * H + (size_t)y;
            unsigned int v = (unsigned int)I_orig[idx] + MASK_FUSE_WEIGHT_FACTOR * (unsigned int)fused_MIP[idx];
            if (v > 255u)
                v = 255u;
            I_orig[idx] = (uint8_t)v;
        }
    }

    return 1;
}
// intersect a polygon edge with row c for trapezoid ROI boundary construction.
int intersect(int x1, int y1, int x2, int y2, int c)
{
    int dy = y2 - y1;
    int dx = x2 - x1;

    if (dy == 0)
    {
        return x1;  
    }

    float num = (c - y1) * dx /(float)dy;

    // Round to nearest integer
    return (int)(x1 + num);
}

// precompute trapezoid ROI zeroing ranges for the current image dimensions.
static void update_trapezoid_roi_boundary_cache(unsigned int pixelCount, unsigned int beamCount)
{
    const int sideRight = NEEDLE_INSERSION_SIDE_RIGHT ? 1 : 0;

    if (trapezoid_roi_cache_valid && trapezoid_roi_cache_pixelCount == pixelCount && trapezoid_roi_cache_beamCount == beamCount && trapezoid_roi_cache_sideRight == sideRight)
        return;

    trapezoid_roi_cache_valid = true;
    trapezoid_roi_cache_pixelCount = pixelCount;
    trapezoid_roi_cache_beamCount = beamCount;
    trapezoid_roi_cache_sideRight = sideRight;

    for (unsigned int col = 0; col < MAX_SCANLINES_NEEDLE; ++col)
    {
        trapezoid_roi_zero_start_row[col] = (int)pixelCount;
        trapezoid_roi_zero_len_rows[col] = 0;
    }

    if (pixelCount <= 1 || beamCount == 0 || beamCount > MAX_SCANLINES_NEEDLE)
        return;

    if (NEEDLE_INSERSION_SIDE_RIGHT)
    {
        int boundaryX = (int)beamCount - (int)(2.8 / 3.56 * (float)beamCount);
        if (boundaryX < 0)
            boundaryX = 0;
        if (boundaryX > (int)beamCount - 1)
            boundaryX = (int)beamCount - 1;

        for (unsigned int col = 0; col < beamCount; ++col)
        {
            int startRow = (int)pixelCount;
            for (unsigned int row = 1; row < pixelCount; ++row)
            {
                int minX = intersect(0, 0, boundaryX, (int)pixelCount - 1, (int)row);
                if ((int)col < minX)
                {
                    startRow = (int)row;
                    break;
                }
            }

            trapezoid_roi_zero_start_row[col] = startRow;
            if (startRow < (int)pixelCount)
                trapezoid_roi_zero_len_rows[col] = pixelCount - (unsigned int)startRow;
        }
    }
    else
    {
        int boundaryX = (int)(2.8 / 3.56 * (float)beamCount);
        if (boundaryX < 0) boundaryX = 0;
        if (boundaryX > (int)beamCount - 1) boundaryX = (int)beamCount - 1;

        for (unsigned int col = 0; col < beamCount; ++col)
        {
            int startRow = (int)pixelCount;
            for (unsigned int row = 1; row < pixelCount; ++row)
            {
                int maxX = intersect((int)beamCount - 1, 0, boundaryX, (int)pixelCount - 1, (int)row);
                if ((int)col > maxX)
                {
                    startRow = (int)row;
                    break;
                }
            }

            trapezoid_roi_zero_start_row[col] = startRow;
            if (startRow < (int)pixelCount)
                trapezoid_roi_zero_len_rows[col] = pixelCount - (unsigned int)startRow;
        }
    }
}

// apply the cached trapezoid ROI mask with one memset per affected column.
static void apply_trapezoid_roi_mask_cached(uint8_t *roi, unsigned int pixelCount, unsigned int beamCount)
{
    update_trapezoid_roi_boundary_cache(pixelCount, beamCount);

    for (unsigned int col = 0; col < beamCount; ++col)
    {
        unsigned int zeroLen = trapezoid_roi_zero_len_rows[col];
        if (zeroLen > 0)
            memset(roi + (size_t)col * pixelCount + (size_t)trapezoid_roi_zero_start_row[col], 0, zeroLen * sizeof(uint8_t));
    }
}
// needle enhancement for one frame
int needle_enhance_process(unsigned int headerLength, int frameNumber_cur, // uint8_t *bmode_data, // H x W, grayscale
                           unsigned int pixelCount_orig, unsigned int beamCount_orig, int needle_len_px, const DepthMaskParams *depth_params_in, const NeedlePipParams *pip_params_in, NeedleLineResult *line)
{
    // Assumimg needle length is 40 mm and maximum needle angle is 45 degree, the effective needle length in the image is 40mm * cos(45) = 28.3 mm.
    // Interrogated frame image is 5 mm in depth, so we can scale down the pixel count to 28.3/5 = 5.66 times to make the PIP faster while still covering the effective needle length in the image.
    // The scaling factor can be adjusted based on the expected needle length and maximum angle.

    unsigned int i;

    // test needle enhancement with external datasets
#if TEST_USING_EXTERNAL_DATA 
        // test consistency between FW  and matlab
        //// LogInfo("original pixelCout  = %u, beamCount = %u: headLength = %u\n", pixelCount_orig, beamCount_orig, headerLength);
        unsigned int pixelCount_inputImg = 384;
        beamCount_orig = 128;
        FILE *filePtr;
        char full_path[256];
        char file_counter_str[64];
        unsigned char imageBuffer[pixelCount_inputImg * beamCount_orig];
        int file_index;

        if (external_dataset_tested == 0)
        {
            file_index = 10008 + binary_imageFile_counter;
            snprintf(file_counter_str, sizeof(file_counter_str), "%d", file_index);

            if (file_index > 10480)
            {
                LogError("All test binary files have been read. Resetting counter to 0.\n");
                binary_imageFile_counter = 0;
                external_dataset_tested++;

                memset(frame_buffer_multiFrames, 0, sizeof(unsigned char) * MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES);
                memset(frame_buffer_multiFrame_idx, -1, sizeof(int) * MAX_NEEDLE_FRAMES);
                buffer_head = 0;
                memset(oldSNRratios, 0, sizeof(uint32_t) * MAX_NEEDLE_FRAMES);
                needle_frame_counter = 0;
                needleMultiFrameState = (NeedleMFState){0};
                needleLogicState = (NeedleLogicState){0};

                // reset tip motion flag and state parameters
                motion_bg_valid = false;
                reset_tip_motion_state();
            }
        }
        else if (external_dataset_tested == 1)
        {
            file_index = 1360 + binary_imageFile_counter;
            snprintf(file_counter_str, sizeof(file_counter_str), "0%d", file_index);
            if (file_index > 1832)
            {
                LogError("All test binary files have been read. Resetting counter to 0.\n");
                binary_imageFile_counter = 0;
                external_dataset_tested++;

                memset(frame_buffer_multiFrames, 0, sizeof(unsigned char) * MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES);
                memset(frame_buffer_multiFrame_idx, -1, sizeof(int) * MAX_NEEDLE_FRAMES);
                buffer_head = 0;
                memset(oldSNRratios, 0, sizeof(uint32_t) * MAX_NEEDLE_FRAMES);
                needle_frame_counter = 0;
                needleMultiFrameState = (NeedleMFState){0};
                needleLogicState = (NeedleLogicState){0};

                // reset tip motion flag and state parameters
                motion_bg_valid = false;
                reset_tip_motion_state();
            }
        }
        else if (external_dataset_tested == 2)
        {
            file_index = 3147 + binary_imageFile_counter;
            snprintf(file_counter_str, sizeof(file_counter_str), "0%d", file_index);
            if (file_index > 3396)
            {
                LogError("All test binary files have been read. Resetting counter to 0.\n");
                binary_imageFile_counter = 0;
                external_dataset_tested++;

                memset(frame_buffer_multiFrames, 0, sizeof(unsigned char) * MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES);
                memset(frame_buffer_multiFrame_idx, -1, sizeof(int) * MAX_NEEDLE_FRAMES);
                buffer_head = 0;
                memset(oldSNRratios, 0, sizeof(uint32_t) * MAX_NEEDLE_FRAMES);
                needle_frame_counter = 0;
                needleMultiFrameState = (NeedleMFState){0};
                needleLogicState = (NeedleLogicState){0};

                // reset tip motion flag and state parameters
                motion_bg_valid = false;
                reset_tip_motion_state();
            }
        }
        else if (external_dataset_tested == 3)
        {
            file_index = 638 + binary_imageFile_counter;
            snprintf(file_counter_str, sizeof(file_counter_str), "00%d", file_index);
            if (file_index > 887)
            {
                LogError("All test binary files have been read. Resetting counter to 0.\n");
                binary_imageFile_counter = 0;
                external_dataset_tested++;

                memset(frame_buffer_multiFrames, 0, sizeof(unsigned char) * MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES);
                memset(frame_buffer_multiFrame_idx, -1, sizeof(int) * MAX_NEEDLE_FRAMES);
                buffer_head = 0;
                memset(oldSNRratios, 0, sizeof(uint32_t) * MAX_NEEDLE_FRAMES);
                needle_frame_counter = 0;
                needleMultiFrameState = (NeedleMFState){0};
                needleLogicState = (NeedleLogicState){0};

                // reset tip motion flag and state parameters
                motion_bg_valid = false;
                reset_tip_motion_state();
            }
        }
        if (external_dataset_tested >= 4)
        {
            LogError("All test binary data sets have been read. Resetting data set counter to 0.\n");
            external_dataset_tested = 0;
            return -1;
        }

        // snprintf adds a null-terminator automatically
        // It returns the number of characters that WOULD have been written
        int len = snprintf(full_path, sizeof(full_path), "%s%siq_data_%s_64_384_0.hex.bin", BINARY_IMAGE_FILE_PATH, test_folders[external_dataset_tested], file_counter_str);
        ////LogInfo("Reading binary image file: %s\n", full_path);

        binary_imageFile_counter++;

        LogInfo("\\CLI full path: %s, imageindex = %d\n", full_path,binary_imageFile_counter );

        filePtr = fopen(full_path, "rb");
        if (filePtr == NULL)
        {
            LogError("File error: Could not open file %s.\n", full_path);
            return -1;
        }

        size_t result = fread(imageBuffer, 1, pixelCount_inputImg * beamCount_orig, filePtr);
        if (result != pixelCount_inputImg * beamCount_orig)
        {
            LogDebug("Reading error: Expected %d bytes, got %zu bytes.\n", pixelCount_inputImg * beamCount_orig, result);
        }
        fclose(filePtr);

        for (i = 0; i < beamCount_orig; ++i)
        {
            size_t offset = headerLength + i * pixelCount_orig;
            memcpy(frame_buffer + offset, imageBuffer + i * pixelCount_inputImg, pixelCount_inputImg * sizeof(uint8_t));
        }

        // incrementing the number of needle frames by one each time a new frame is acquired when needle is on
        needle_frame_counter = binary_imageFile_counter - 1; // temporarily assign the number of external file read in to the needle_frame_counter for testing purpose
        needle_detection_physical_frame_counter = needle_frame_counter - 1;
#endif

    if (needle_frame_counter == 0 && needle_detection_physical_frame_counter != 0)
    {
        // feature-level reset happened in resetNeedleConfidenceAndMotionVariables(). Restart both the physical scheduler and cached virtual detection state.
        reset_needle_detection_cache_state();
        needle_detection_physical_frame_counter = 0;
    }
    needle_detection_physical_frame_counter++;

    unsigned int imageDepth = probeState.depth;

    // unsigned int pixelCount =(unsigned int)((float)pixelCount_orig * 2.8/5.0);
    unsigned int pixelCount = (unsigned int)((float)pixelCount_orig * NEEDLE_LEN_MM * 0.71 * MM_TO_PIXEL_CONVERSION_FACTOR / imageDepth); // needle length/sqrt(2) for 45 degree angle
    unsigned int beamCount = beamCount_orig;
    unsigned long N = (unsigned long)pixelCount * (unsigned long)beamCount;

    if ((unsigned long)headerLength + N > RAW_DATA_SIZE_MAX || pixelCount > MAX_SAMPLE_SIZE_NEEDLE || beamCount > MAX_SCANLINES_NEEDLE)
    {
        LogDebug("Needle enhance process: out of bound error. H = %u W = %u exceeds static buffers (%u x %u)\n", pixelCount, beamCount, MAX_SAMPLE_SIZE_NEEDLE, MAX_SCANLINES_NEEDLE);
        reset_needle_detection_cache_state();
        return -1;
    }

    NeedlePipParams p = needleDetectionParams; // normalize=0 (already normalized)
    if (pip_params_in)
        p = *pip_params_in;

    bool cacheUsable = needle_detection_cache_is_usable(pixelCount, beamCount, needle_len_px);
    bool scheduledDetectionFrame = (NEEDLE_DETECTION_INTERVAL_FRAMES <= 1) || (((needle_detection_physical_frame_counter - 1) % NEEDLE_DETECTION_INTERVAL_FRAMES) == 0);
    bool runDetectionThisFrame = scheduledDetectionFrame;

    if (!runDetectionThisFrame && !cacheUsable)
    {
        // physical frame is between virtual detections. Do not run the detector here. Without an active virtual status there is nothing reliable to fuse this frame.
        *line = (NeedleLineResult){0};

        LogDebug("Skipping physical frame: no active virtual needle status available.\n");

        return -1;
    }

    // copy the pixel data within roi from current frame into frame_buffer_roi for fusion. The trapezoid ROI is only needed on frames where the expensive detector runs.
    for (i = 0; i < beamCount; ++i)
    {
        memcpy(frame_buffer_roi + pixelCount * i, frame_buffer + headerLength + pixelCount_orig * i, pixelCount * sizeof(uint8_t));

        if (runDetectionThisFrame)
            memcpy(frame_buffer_traperoid_roi + pixelCount * i, frame_buffer + headerLength + pixelCount_orig * i, pixelCount * sizeof(uint8_t));
    }

    if (runDetectionThisFrame)
        apply_trapezoid_roi_mask_cached(frame_buffer_traperoid_roi, pixelCount, beamCount);

    unsigned int Wout = 0;
    unsigned int Hout = 0;

    if (runDetectionThisFrame)
    {
        // advance the expensive detector stream only on virtual frames.
        needle_frame_counter++;

        // square maximum buffer side for all rotations without cropping the original image (can be optimized)
        unsigned int max_side_all_rotation = (unsigned int)(ceil(sqrt((double)pixelCount * pixelCount + (double)beamCount * beamCount)) + 2);
        if (max_side_all_rotation > MAX_SAMPLE_SIZE_NEEDLE)
        {
            LogDebug("Needle enhance: outN=%u > MAX_SAMPLE_SIZE_NEEDLE = %u\n", max_side_all_rotation, MAX_SAMPLE_SIZE_NEEDLE);
            reset_needle_detection_cache_state();
            return -1;
        }
        else
        {
            if (max_side_all_rotation < pixelCount)
                max_side_all_rotation = pixelCount;
            if (max_side_all_rotation < beamCount)
                max_side_all_rotation = beamCount;

            latest_virtual_conf_frame_valid = false;
            latest_virtual_conf_frame = 0.0f;
            *line = pip_with_prior_info(frame_buffer_traperoid_roi, pixelCount, beamCount, needle_len_px, &p, max_side_all_rotation, &Hout, &Wout, depth_params_in);

            if (line->detected == false)
            {
                // a virtual frame can have a valid geometric line before the temporal confidence gate has fully turned on. 
                // Cache that geometry so cheap physical-frame confidence can continue building the gate.
                if (needle_confidence_on && line->conf_score > 0.0f && cached_line_geometry_is_usable(line, Hout, Wout))
                {
                    update_needle_detection_cache(line, Hout, Wout, pixelCount, beamCount, needle_len_px, frameNumber_cur);
                    LogDebug("Needle line cached while confidence gate is still off.\n");
                }
                else
                {
                    LogDebug("Needle not detected on virtual detection frame\n");
                    reset_needle_detection_cache_state();
                    return -1;
                }
            }
            else
            {
                update_needle_detection_cache(line, Hout, Wout, pixelCount, beamCount, needle_len_px, frameNumber_cur);
            }
        }
    }
    else
    {
        use_cached_needle_detection(line, &Hout, &Wout);

        if (needle_confidence_on)
        {
#if NEEDLE_HOLD_VIRTUAL_CONFIDENCE_ON_SKIPPED_FRAMES
            float cachedConf = needle_detection_cache_state.hasHeldConfFrame ? needle_detection_cache_state.heldConfFrame : cached_line_confidence_current_frame(frame_buffer_roi, pixelCount, beamCount, line, Hout, Wout);
#else
            float cachedConf = cached_line_confidence_current_frame(frame_buffer_roi, pixelCount, beamCount, line, Hout, Wout);
#endif
            needle_line_confidence_multiFrames(cachedConf, *line);
            line->conf_score = needleMultiFrameState.confSmooth;
            line->detected = needleMultiFrameState.enhancementActive;
        }
        else
            line->detected = true;

#if NEEDLE_HOLD_VIRTUAL_CONFIDENCE_ON_SKIPPED_FRAMES
        LogDebug("Skipping needle detection and updating gate from held virtual-frame confidence.\n");
#else
        LogDebug("Skipping needle detection and updating gate from cached-line confidence.\n");
#endif
    }
    LogDebug("Needle detection result rho = %d, theta = %f, entry_x = %d, entry_y = %d, tip_x %d, tip_y = %d, physicalFrame = %u, virtualFrame = %d, detected = %d\n", line->best_rho, line->theta_deg, line->entry_x, line->entry_y, line->tip_x, line->tip_y, needle_detection_physical_frame_counter, needle_frame_counter, line->detected);

    if (line->detected == false)
    {
        LogDebug("Needle enhance: line not detected, skip fusion\n");
        return -1;
    }

    if (Hout > 0 && Wout > 0 && Hout <= MAX_SAMPLE_SIZE_NEEDLE && Wout <= MAX_SAMPLE_SIZE_NEEDLE)
    {
        if (fuse_needle_mask_optimized(frameNumber_cur, frame_buffer_roi, pixelCount, beamCount, line, Hout, Wout) == -1)
        {
            LogDebug("Fuse needle mask failed\n");
            return -1;
        }

        // copy the fused pixel data within roi back to the current frame.
        for (i = 0; i < beamCount; ++i)
            memcpy(frame_buffer + headerLength + pixelCount_orig * i, frame_buffer_roi + pixelCount * i, pixelCount * sizeof(uint8_t));
    }
    else
    {
        LogDebug("Needle enhance: cached/detected rotated size invalid Hout=%u Wout=%u\n", Hout, Wout);
        return -1;
    }

    return 1;
}
