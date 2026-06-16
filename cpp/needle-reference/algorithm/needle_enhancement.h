#ifndef NEEDLE_ENHANCEMENT_H_
#define NEEDLE_ENHANCEMENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// maximum kernel size for gaussian filter
#define MAX_KERNEL_SIZE 63

#define MAX_SAMPLE_SIZE_NEEDLE 1024
#define MAX_SCANLINES_NEEDLE 128

// number of needle frames to calculate baseline SNR, noise floor and other relevent baseline threshold for the current frame data
#define NEEDLE_FRAMES_BASELINE 100

// needle length, mm to pixel converter factor (different presets have different factors) and needle length factor (needle length multiplying this factor 
// is used for needle detection and depth mask (prior info))
#define NEEDLE_LEN_MM  38 // mm
#define MM_TO_PIXEL_CONVERSION_FACTOR 7.7922  // different presets have different values
#define NEEDLE_LEN_FACTOR 1.5
#define NEEDLE_INSERSION_SIDE_RIGHT 1

// forward-profile line detector: distribute each original pixel to rotated rho bins.
#define NEEDLE_DETECTION_ACCUM_USE_Y_BILINEAR 1

// Virtual detection stream mode: run expensive needle line/tip/confidence detection once
// every N frames, while fusion still runs every frame with the latest valid line.
#define NEEDLE_DETECTION_INTERVAL_FRAMES 3

// virtual-frame confidence gate compensation. Detection/confidence update only happens every NEEDLE_DETECTION_INTERVAL_FRAMES physical frames, so ON counters
// are scaled down for fast acquisition while OFF counters remain unscaled for slow, stable turn-off.
#define NEEDLE_VIRTUAL_GATE_SCALE_ON_COUNTS 1
#define NEEDLE_VIRTUAL_GATE_FAST_ON_SLOW_OFF 1
#define NEEDLE_VIRTUAL_GATE_SCALE_EMA_ALPHA 1

// normalized threshold for needle tip detection
#define NEEDLE_ENDPOINTS_THRESHOLD 0.4
#define NEEDLE_DETECTION_CONF_THRESHOLD 0.42

// maximum rows of pixels at the bottom of the frame image to be masked out before needle detection
#define MAX_BOTTOM_ROW_TO_MASK 40

// needle width
#define NEEDLE_WIDTH  1

// needle mask weighting factor for fusion
#define MASK_FUSE_WEIGHT_FACTOR 5 //3

// maximum number of frames to use for needle detection and multi-frame confidence score calculation (e.g. temporal smoothing)
#define MAX_NEEDLE_FRAMES 3  // using the current frame and 2 previous frames for  confidence score calculation and temporal smoothing
#define MAX_NEEDLE_SIZE_PIXELS 1024 // maximum possible needle length in pixels, used for static array allocation in needle_line_confidence function, can be adjusted based on the actual image size and needle length in pixels
#define BAND_R 1  // band radius in pixels for local max search around the detected line in the rotated image for confidence score calculation
#define REFINEMENT_OFFSET_for_DETECTED_NEEDLE  10 // offset used for refinement of the detected needle line,
#define NEEDLE_LINE_HALF_BAND 1
#define NEEDLE_TIP_SMOOTH_FRACTION 0.10f
#define NEEDLE_TOP_LINE_CANDIDATE_COUNT 3
#define NEEDLE_TOP_LINE_CANDIDATE_ROWS_PER_ANGLE 1
#define NEEDLE_TOP_LINE_MIN_SEGMENT_PIXELS 12
#define NEEDLE_TOP_LINE_REFINE_RHO_BEFORE_RESCORE 0
#define NEEDLE_ENABLE_FINAL_LINE_REFINEMENT 1
#define NEEDLE_REJECT_ENTRY_TOO_DEEP 1
#define NEEDLE_MAX_ENTRY_ORIG_Y_PX 120
#define NEEDLE_MAX_ENTRY_ORIG_Y_FRACTION 0.40f

#define NEEDLE_ENABLE_MOTION_TIP_CORRECTION 1
#define NEEDLE_MOTION_EMA_ALPHA 0.25f
#define NEEDLE_MOTION_SEARCH_RADIUS_PX 55
#define NEEDLE_MOTION_MIN_CORRECTION_PX 4.0f
#define NEEDLE_MOTION_MAX_CORRECTION_PX 60.0f
#define NEEDLE_MOTION_ABOVE_ROWS 4
#define NEEDLE_MOTION_BELOW_ROWS 40
#define NEEDLE_MOTION_BOX_RADIUS 1
#define NEEDLE_MOTION_SCORE_SMOOTH_W 5
#define NEEDLE_MOTION_ROBUST_KSIGMA 3.0f
#define NEEDLE_MOTION_PEAK_FRAC 0.45f
#define NEEDLE_MOTION_SCORE_KSIGMA 3.0f
#define NEEDLE_MOTION_MIN_SCORE_CONFIDENCE 1.0f
#define NEEDLE_MOTION_MIN_HOT_AREA 12
#define NEEDLE_MOTION_MIN_HOT_ROWS 2
#define NEEDLE_MOTION_MIN_HOT_COLS 2
#define NEEDLE_MOTION_MIN_LOCAL_PEAK_SIGMA 4.0f
#define NEEDLE_MOTION_MIN_LOCAL_PEAK_ABOVE_THR_SIGMA 0.75f
#define NEEDLE_MOTION_CENTROID_HALF_WINDOW_PX 4
#define NEEDLE_MOTION_BLOB_MIN_AREA 16
#define NEEDLE_MOTION_BLOB_MIN_WIDTH 2
#define NEEDLE_MOTION_BLOB_MIN_HEIGHT 3
#define NEEDLE_MOTION_BLOB_MAX_WIDTH 22
#define NEEDLE_MOTION_BLOB_MAX_HEIGHT 36
#define NEEDLE_MOTION_BLOB_MIN_PEAK_SIGMA 6.0f
#define NEEDLE_MOTION_BLOB_MIN_PEAK_ABOVE_THR_SIGMA 1.25f
#define NEEDLE_MOTION_BLOB_MIN_MEAN_ABOVE_THR_SIGMA 0.75f
#define NEEDLE_MOTION_BLOB_MIN_SUM_ABOVE_THR_SIGMA 20.0f
#define NEEDLE_MOTION_TRACK_MAX_JUMP_PX 18.0f
#define NEEDLE_MOTION_TRACK_ACQUIRE_MAX_DIST_PX 60.0f
#define NEEDLE_MOTION_TRACK_DISTANCE_PENALTY 1.5f
#define NEEDLE_MOTION_TRACK_ALPHA 0.45f
#define NEEDLE_MOTION_TRACK_VELOCITY_ALPHA 0.30f
#define NEEDLE_MOTION_TRACK_MAX_MISS_FRAMES 3
#define NEEDLE_MOTION_TRACK_MIN_CONFIRM_FRAMES 1
#define NEEDLE_MOTION_TRACK_LINE_THETA_JUMP_DEG 4.0f
#define NEEDLE_MOTION_TRACK_LINE_RHO_JUMP_PX 12

// confidence score calculation for the detected needle line
#define Q15_ONE 32767u // for Q15 fixed point operation
#define NEEDLE_SHAFT_INTENSITY_P20_MIN 70 // 75 minimum 20 percentile value for needle shaft intensity value.
#define NEEDLE_SHAFT_INTENSITY_P50_MIN 115 // 120 minimum 50 percentile threshold value for needle intesity value.
#define NEEDLE_SHAFT_INTENSITY_P85_MIN 150 // 150 minimum 850 percentile threshold value for needle intesity value.
#define REVERB_LINE_COUNT 10
#define REVERB_OFFLINE   20
#define REVERB_PSD_NFFT  128
#define REVERB_PSD_BINS  (REVERB_PSD_NFFT / 2 + 1)
#define CONF_BASE_THRESH 0.2
#define MIN_VALID_NEEDLE_SIZE_PIXELS 15 // minimum needle length in pixels before applying confidence score calculation
#define NEEDLE_CACHED_LINE_CONFIDENCE_EVERY_FRAME 1
#define NEEDLE_HOLD_VIRTUAL_CONFIDENCE_ON_SKIPPED_FRAMES 0

// PIP search parameters used by the needle line detector.
typedef struct {
    float theta_step_deg;       // e.g. 1.0
    float theta_range_min_deg;  // e.g. 0.0
    float theta_range_max_deg;  // e.g. 179.0
    float resize_factor;        // e.g. 1.0 
    bool   normalize;           // flag indicating if normalizing the input image before needle detection (true: normalize inside PIP, false: use as-is)
} NeedlePipParams;

// Depth masking parameters applied before line detection.
typedef struct {
    bool  mask_skin_layer;         // flag indicates whether to mask out the skin layer (1 = mask top  depth_mask_thickness_px rows of the image before detect needle)
    unsigned int  depth_mask_thickness_px; // number of pixel to be masked out at the skin layer
} DepthMaskParams;

// Needle line and tip result in rotated-image coordinates.
typedef struct {
    int best_rho;        // row index in rotated image (0-based)
    float theta_deg;  // needle angle in degrees relative (counter clockwise) to positive x axis
    int entry_x;    // entry point in rotated coords
    int entry_y;
    int tip_x;      // tip point in rotated coords
    int tip_y;
    bool detected;   // flag indicates if needle is detected
    float conf_score; // confidence score of the detected needle line
} NeedleLineResult;

// Per-frame evidence terms used to form a line confidence score.
typedef struct
{
    uint16_t  thr_low;
    uint16_t  thr_high;
    uint16_t  tsat;

    float support;
    float continuity;
    float strength;
    float reverb_score;
    float reverb_ratio;

    uint16_t n_total;
    uint16_t n_hit;
    uint16_t longest_run;
} NeedleLineEvidence;

// Single-frame confidence output and its underlying evidence.
typedef struct
{
    NeedleLineEvidence centerEv;
    float confidence_score_singleFrame;
} NeedleConfidenceSingleFrame;

//multiframe confidence score calculation for the detected needle line based on the confidence scores from multiple frames, where the input confidence score is in Q15 fixed point format and the output confidence score is in Q15 fixed point format.
typedef struct
{
    float emaAlpha;
    float alpha_slow;
    float confidence_thr;
    int32_t  max_rho_jump;
    float  max_theta_deg_jump;
    float T_on;
    float T_off;
    uint8_t  K_on;
    uint8_t  K_off;
} NeedleMFConfig;

// multi-frame tracking and confidence state carried across virtual frames.
typedef struct
{
    uint8_t  initialized;
    uint8_t  tracked;
    uint8_t  onCount;
    uint8_t  offCount;
    float confSmooth;
    int32_t smoothRho;
    float smoothTheta_deg;
    float smoothTip_x;
    bool enhancementLogicOn; // flag to indicate if needle enhancement is on based on live logic
    bool enhancementActive; // flag to indicate if needle enhancement is active based on the multi-frame and logic
} NeedleMFState;

// STA/LTA and z-score normalization for the multi-frame confidence score for better temporal stability and consistency
// Live confidence gate states for STA/LTA temporal logic.
typedef enum
{
    NEEDLE_STATE_OFF   = 0,
    NEEDLE_STATE_ARMED = 1,
    NEEDLE_STATE_ON    = 2,
    NEEDLE_STATE_DECAY = 3
} NeedleLogicMode;

// STA/LTA gate state used by update_needle_logic_live().
typedef struct
{
    uint8_t initialized;
    bool no_need_to_reset;  // true means no reset needed, false means to reset (kind of counter_intuition, but convenient for implementation and initialization)

    float sta;
    //float lta;
    float lta_on;
    float lta_off;

    float ratio_mean_off;
    float ratio_var_off;

    NeedleLogicMode state;

    uint16_t arm_count;
    uint16_t confirm_count;
    uint16_t decay_count;
    uint16_t off_count;

    float peak_sta_on;
    float peak_conf_on;
} NeedleLogicState;

extern int needle_frame_counter;
extern NeedlePipParams needleDetectionParams;
extern DepthMaskParams depthMaskParams;

// tip motion parameters
extern bool motion_bg_valid;

// confidence parameters
extern int frame_buffer_multiFrame_idx[MAX_NEEDLE_FRAMES]; // frame indices for the frames stored in frame_buffer_multiFrames, initialized to -1 indicating no valid frame stored yet
extern unsigned char frame_buffer_multiFrames[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES];
extern uint32_t multi_frame_Weights[MAX_NEEDLE_FRAMES]; // weights for current frame, previous frame 1 and previous frame 2, in Q8.8 fixed point format, assuming MAX_PREVIOUS_NEEDLE_FRAMES is 2
extern int buffer_head; // head index for the circular buffer of multi-frame storage
extern NeedleMFState needleMultiFrameState; // needle multi-frame state for confidence score calculation, initialized to 0
extern bool needle_confidence_on;

// Clamp an integer to a closed interval.
static inline int clampi(int v, int lo, int hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

// Clamp a float to a closed interval.
static inline float clampf(float x, float lo, float hi)
{
    return fminf(fmaxf(x, lo), hi);
}

// reset temporal state used by motion-based tip correction.
void reset_tip_motion_state(void);

// initialize global needle confidence score parameters when needle feature is turned on
void resetNeedleConfidenceAndMotionVariables(unsigned int value);

// calculate confidence score for one detected line and update temporal confidence state.
NeedleConfidenceSingleFrame needle_line_confidence_singleFrame(float *Irot_best, int Hrot_best, int Wrot_best, int stride_rot, NeedleLineResult *best_needle_line);
void needle_line_confidence_multiFrames(float conf_frame, NeedleLineResult best_needle_line_singleFrame);

// top-level needle enhancement API called once per physical frame.
int needle_enhance_process(unsigned int headerLength, // H x W, grayscale frame image
                            int frameNumber_cur, unsigned int H, unsigned int W, int needle_len_px,
                            const DepthMaskParams *depth_params_in, const NeedlePipParams *pip_params_in, NeedleLineResult *line_out);

// the following parameters are used for testing against external dataset.
#define TEST_USING_EXTERNAL_DATA 0  // flag to use external test datasets for testing
#define BINARY_IMAGE_FILE_PATH  "/run/media/mmcblk0p1/"
extern uint32_t oldSNRratios[MAX_NEEDLE_FRAMES]; // SNR ratios from the previous frames, in Q8.8 fixed point format
extern NeedleMFState needleMultiFrameState; // state for multi-frame confidence score calculation, initialized to 0
extern NeedleLogicState needleLogicState;

#endif /* NEEDLE_ENHANCEMENT_H_ */
