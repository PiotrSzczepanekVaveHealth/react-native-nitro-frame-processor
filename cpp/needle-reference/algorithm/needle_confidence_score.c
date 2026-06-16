#include <stdint.h>
#include <stdbool.h>

#include "needle_enhancement.h"
#include "../../debug.h"

// needle confidence score calculation for the detected needle line in a single frame based on the evidence from the local max values along the detected line in the rotated image and the reverberation score, where the input image is in float format and the output confidence score is in Q15 fixed point format.
bool needle_confidence_on = false;
unsigned char frame_buffer_multiFrames[MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES];
uint32_t multi_frame_Weights[MAX_NEEDLE_FRAMES] = {1, 2, 3}; // weights for current frame, previous frame 1 and previous frame 2, in Q8.8 fixed point format, assuming MAX_PREVIOUS_NEEDLE_FRAMES is 2
int frame_buffer_multiFrame_idx[MAX_NEEDLE_FRAMES] = {-1}; // frame indices for the frames stored in frame_buffer_multiFrames, initialized to -1 indicating no valid frame stored yet
int buffer_head = 0; // head index for the circular buffer of multi-frame storage
int needle_frame_counter = 0; // frame counter since needle feature is on
uint32_t oldSNRratios[MAX_NEEDLE_FRAMES] = {0}; // SNR ratios from the previous frames, in Q8.8 fixed point format
NeedleMFState needleMultiFrameState = {0}; // state for multi-frame confidence score calculation, initialized to 0
NeedleLogicState needleLogicState = {0}; // state for needle logic, initialized to 0

static uint16_t lm[MAX_NEEDLE_SIZE_PIXELS]; // local max values along the detected line in the rotated image within a band around the detected line (bandR)
static uint16_t lm_sorted[MAX_NEEDLE_SIZE_PIXELS]; // sorted local max values for percentile calculation
uint16_t vals[REVERB_OFFLINE * 2 + 1]; // values for reverberation score calculation, with size based on the maximum possible number of pixels in the vertical direction for the line profile used in reverberation score calculation

// hamming window coefficient (fixed-point)
static const uint16_t hamming41_q15[41] =
{
     2621,  2807,  3359,  4264,  5500,  7036,  8835, 10851, 13036, 15336,
    17694, 20052, 22352, 24537, 26554, 28352, 29888, 31124, 32029, 32581,
    32767,
    32581, 32029, 31124, 29888, 28352, 26554, 24537, 22352, 20052, 17694,
    15336, 13036, 10851,  8835,  7036,  5500,  4264,  3359,  2807,  2621
};

// coeffients for Goertzel power calculation
static const int16_t coef2cos_q14_128[65] =
{
     32767,  32729,  32610,  32413,  32138,  31786,  31357,  30853,
     30274,  29622,  28899,  28106,  27246,  26320,  25330,  24279,
     23170,  22006,  20788,  19520,  18205,  16846,  15447,  14010,
     12540,  11039,   9512,   7962,   6393,   4808,   3212,   1608,
         0,  -1608,  -3212,  -4808,  -6393,  -7962,  -9512, -11039,
    -12540, -14010, -15447, -16846, -18205, -19520, -20788, -22006,
    -23170, -24279, -25330, -26320, -27246, -28106, -28899, -29622,
    -30274, -30853, -31357, -31786, -32138, -32413, -32610, -32729,
    -32768
};

static NeedleMFConfig cfg = 
{
    emaAlpha           : 0.25,
    alpha_slow         : 0.05,
    confidence_thr     : 0.45,
    max_rho_jump       : 10,
    max_theta_deg_jump : 4,
    T_on               : 0.5,
    T_off              : 0.35,
    K_on               : 3,
    K_off              : 3
};

// tunable parameters for needle logic, can be adjusted based on the actual application scenario and requirements
typedef struct
{
    float alpha_sta;
    float alpha_lta;
    float beta_ratio_off;
    float lta_floor;
    float sigma_ratio_min;

    // arm
    float z_arm;
    float r_arm;
    uint8_t k_arm;

    // confirm stable plateau
    float sta_on;
    float lta_on;
    float r_confirm;
    uint8_t k_confirm;

    // decay entry
    float sta_decay;
    float r_decay;
    float z_decay;
    uint8_t k_decay;

    // off entry
    float sta_off;
    float r_off;
    float z_off;
    uint8_t k_off;
} NeedleLogicParams;

// current parameters used for needle logic update
static const NeedleLogicParams kNeedleLogicParams =
{
    0.25f, 0.03f,
    0.03f, 0.05f, 0.05f,
    2.0f, 1.10f, 2u,
    0.52f, 0.45f, 1.03f, 3u,
    0.56f, 0.94f, -0.5f, 2u,
    0.54f, 0.90f, -1.0f, 3u
};

static const float kNeedleLogicAlphaLtaOn = 0.10f;

// Return the effective gate update interval for virtual-frame or every-frame confidence mode.
static unsigned int virtual_gate_interval(void)
{
#if NEEDLE_CACHED_LINE_CONFIDENCE_EVERY_FRAME
    // Cached-line confidence updates the gate on every physical frame, so do
    // not compress ON counters or inflate STA/LTA alphas as if updates only
    // arrived every virtual detection frame.
    return 1U;
#else
    unsigned int interval = (unsigned int)NEEDLE_DETECTION_INTERVAL_FRAMES;
    return (interval < 1U) ? 1U : interval;
#endif
}
// Scale ON counters so virtual-frame confidence does not delay activation too much.
static uint8_t virtual_gate_fast_on_count(uint8_t k)
{
#if NEEDLE_VIRTUAL_GATE_SCALE_ON_COUNTS
        unsigned int interval = virtual_gate_interval();
        if (interval <= 1U)
            return k;

        unsigned int scaled = ((unsigned int)k + 2 * interval - 1U) / interval;
        if (scaled < 1U) scaled = 1U;
        if (scaled > 255U) scaled = 255U;
        return (uint8_t)scaled;
#else
        return k;
#endif
}
// Keep or scale OFF counters depending on the selected slow-off virtual gate mode.
static uint8_t virtual_gate_slow_off_count(uint8_t k)
{
#if NEEDLE_VIRTUAL_GATE_FAST_ON_SLOW_OFF
        return k;
#else
        return virtual_gate_fast_on_count(k);
#endif
}

// Convert an EMA alpha to an equivalent value for sparse virtual-frame updates.
static float virtual_gate_scaled_alpha(float alpha)
{
#if NEEDLE_VIRTUAL_GATE_SCALE_EMA_ALPHA
        unsigned int interval = virtual_gate_interval();
        float keep;
        float keepPow;

        if (interval <= 1U)
            return alpha;

        if (alpha <= 0.0f)
            return 0.0f;
        if (alpha >= 1.0f)
            return 1.0f;

        keep = 1.0f - alpha;
        keepPow = 1.0f;
        for (unsigned int i = 0; i < interval; ++i)
            keepPow *= keep;

        return 1.0f - keepPow;
#else
        return alpha;
#endif
}

// This is used for debugging purposed, currently is turned off
typedef struct
{
    float sta;
    float lta;
    float ratio;
    float ratio_mean_off;

    int   state;
    uint16_t arm_count;
    uint16_t confirm_count;
    uint16_t decay_count;
    uint16_t off_count;

    float STA_on;
    float LTA_on;
    float STA_decay;
    float STA_off;
    float R_arm;
    float R_confirm;
    float R_decay;
    float R_off;
    float Z_arm;
    float Z_decay;
    float Z_off;

    float peak_sta_on;
    float peak_conf_on;
} NeedleLogicDebug;

// initialize global needle confidence score parameters when needle feature is turned on
void resetNeedleConfidenceAndMotionVariables(unsigned int value)
{
    // initialize the necessary variables for needle confidence calculation.
    memset(frame_buffer_multiFrames, 0, sizeof(unsigned char) * MAX_SAMPLE_SIZE_NEEDLE * MAX_SCANLINES_NEEDLE * MAX_NEEDLE_FRAMES);
    memset(frame_buffer_multiFrame_idx, -1, sizeof(int) * MAX_NEEDLE_FRAMES) ;
    buffer_head = 0; // head index for the circular buffer of multi-frame storage
    memset(oldSNRratios, 0, sizeof(uint32_t) * MAX_NEEDLE_FRAMES) ; // SNR ratios from the previous frames, in Q8.8 fixed point format

    needle_frame_counter = 0;
    
    // initialize multiframe state parameters
    needleMultiFrameState = (NeedleMFState){0};

    // initialize needle logic reset flag and state parameters
    needleLogicState = (NeedleLogicState){0};

    if(value == 2)  // turn on confidence activation
        needle_confidence_on = true;
    else  // fusion is always on
        needle_confidence_on = false;

    // reset tip motion flag and state parameters
    motion_bg_valid = false;
    reset_tip_motion_state();
}

// Convert a float value in the range [0, 255] to unsigned Q8.8 fixed point format, where the output is a 16-bit unsigned integer with 8 bits for the integer part and 8 bits for the fractional part.
static inline uint16_t float_to_ushort(float x)
{
    if (x <= 0.0f)
        return 0;
    if (x >= 255.0f)
        return (uint16_t)(255u << 8);

    return (uint16_t)(x * 256.0f + 0.5f);
}


// Calculate mean of the top three local values in a small band around the line sample.
static uint16_t local_top3_mean(const float *Irot_best, int Hrot_best, int Wrot_best, int stride_rot, int x, int y, int bandR)
{
    int x0 = clampi(x - bandR, 0, Wrot_best - 1);
    int x1 = clampi(x + bandR, 0, Wrot_best - 1);
    int y0 = clampi(y - bandR, 0, Hrot_best - 1);
    int y1 = clampi(y + bandR, 0, Hrot_best - 1);

    uint16_t top1 = 0, top2 = 0, top3 = 0;

    for (int xx = x0; xx <= x1; ++xx)
    {
        uint32_t base = (uint32_t)xx * (uint32_t)stride_rot;

        for (int yy = y0; yy <= y1; ++yy)
        {
            uint16_t v = float_to_ushort(Irot_best[base + (uint32_t)yy]);

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
                top3 = v;
        }
    }

    return (uint16_t)(((uint32_t)top1 + (uint32_t)top2 + (uint32_t)top3 + 1u) / 3u);
}

// calculate the value at the given percentile in the histogram, where the percentile is based on the total number of samples in the histogram, not the number of bins.
static uint16_t percentile_sorted(uint16_t *a, int n, uint8_t pct)
{
    if (n <= 0)
        return 0;

    uint32_t rank = ((uint32_t)pct * (uint32_t)n + 99u) / 100u;  // ceil(pct*n/100)

    if (rank < 1u)
        rank = 1u;
    if (rank > (uint32_t)n)
        rank = (uint32_t)n;

    return a[rank - 1u];
}

// fill interior false gaps of length <= maxGap in a boolean mask 
static void fill_small_gaps(const uint8_t *kept, int len, int maxGap, uint8_t *kept_filled)
{
    int i, j, gapLen;
    bool leftOn, rightOn;

    for (i = 0; i < len; ++i)
        kept_filled[i] = kept[i];

    i = 0;
    while (i < len)
    {
        if (kept_filled[i])
        {
            ++i;
            continue;
        }

        j = i;
        while (j < len && !kept_filled[j])
            ++j;

        gapLen = j - i;

        leftOn  = (i > 0)   && (kept_filled[i - 1] != 0);
        rightOn = (j < len) && (kept_filled[j] != 0);

        if (leftOn && rightOn && gapLen <= maxGap)
        {
            for (int k = i; k < j; ++k)
                kept_filled[k] = 1u;
        }

        i = j;
    }
}

// longest run count in a boolean mask
static uint16_t longest_run_count(const uint8_t *hit, int len)
{
    int i = 0;
    int best = 0;

    while (i < len)
    {
        if (!hit[i])
        {
            ++i;
            continue;
        }

        int j = i;
        while (j < len && hit[j])
            ++j;

        if ((j - i) > best)
            best = (j - i);

        i = j;
    }

    return (uint16_t)best;
}

// calculate support, strength, and continuity of a detected needle line
static NeedleLineEvidence needle_line_confidence(const float *Irot_best, int Hrot_best, int Wrot_best, int stride_rot /* column major stride */, NeedleLineResult best_needle_line, int bandR)
{
    NeedleLineEvidence ev = {0};
    int32_t span;
    int32_t p20, p90;
    int32_t thr_low, thr_high, tsat;
    uint8_t kept_mask_local[MAX_NEEDLE_SIZE_PIXELS];
    uint8_t kept_filled_local[MAX_NEEDLE_SIZE_PIXELS];

    int len =  best_needle_line.entry_x - best_needle_line.tip_x + 1;
    if (len <= 0 || len > MAX_NEEDLE_SIZE_PIXELS) // if the detected line is too short or too long, which is unlikely to be a valid needle line, return low confidence score directly without further calculation to save computation
    {   
        ev.thr_low = (uint16_t)(255u << 8);
        ev.thr_high = (uint16_t)(255u << 8);
        ev.tsat = (uint16_t)(255u << 8);
        return ev;
    } 
    ev.n_total = len;

    if (len < MIN_VALID_NEEDLE_SIZE_PIXELS)
    {
        ev.thr_low = (uint16_t)(255u << 8);
        ev.thr_high = (uint16_t)(255u << 8);
        ev.tsat = (uint16_t)(255u << 8);
        ev.support = 0.0f;
        ev.continuity = 0.0f;
        ev.strength = 0.0f;
        ev.n_hit = 0u;
        ev.longest_run = 0u;
        return ev;
    }

    // calculate histogram of local max values along the detected line in the rotated image within a band around the detected line (bandR)
    for (int x = best_needle_line.tip_x; x <= best_needle_line.entry_x; ++x)
    {
        int index = x - best_needle_line.tip_x;  
        lm[index] = local_top3_mean(Irot_best, Hrot_best, Wrot_best, stride_rot, x, best_needle_line.tip_y, bandR);
        lm_sorted[index] = lm[index];
    }

    //insertion sort lm
    for (int i = 1; i < len; ++i)
    {
        uint16_t key = lm_sorted[i];
        int j = i - 1;
    
        while (j >= 0 && lm_sorted[j] > key)
        {
            lm_sorted[j + 1] = lm_sorted[j];
            --j;
        }
    
        lm_sorted[j + 1] = key;
    }
    p20 = percentile_sorted(lm_sorted, len, 20); // UQ8.8
    p90 = percentile_sorted(lm_sorted, len, 90); // UQ8.8
    span = p90 - p20;
    if (span < 1)
        span = 1;

    {
        /* eta_q8 = eta * 256 */
        uint32_t eta_q8 = (uint32_t)(((int64_t)span << 8) / (int64_t)((p90 > 1) ? p90 : 1));
        uint32_t m_low_q8  = (20u  * eta_q8 + 50u) / 100u;
        uint32_t m_high_q8 = (50u  * eta_q8 + 50u) / 100u;
        uint32_t m_sat_q8  = (85u  * eta_q8 + 50u) / 100u;

        thr_low  = p20 + (int32_t)(((int64_t)m_low_q8  * (int64_t)span + 128) >> 8);
        thr_high = p20 + (int32_t)(((int64_t)m_high_q8 * (int64_t)span + 128) >> 8);
        tsat     = p20 + (int32_t)(((int64_t)m_sat_q8  * (int64_t)span + 128) >> 8);
    }

    // calculate thresholds based on the percentile and span of the values at the local max along the detected line to boost confidence for uniform needle shaft
    // floors in UQ8.8 
    if (thr_low  < (NEEDLE_SHAFT_INTENSITY_P20_MIN << 8))
        thr_low  = (NEEDLE_SHAFT_INTENSITY_P20_MIN << 8);
    if (thr_high < (NEEDLE_SHAFT_INTENSITY_P50_MIN << 8))
        thr_high = (NEEDLE_SHAFT_INTENSITY_P50_MIN << 8);
    if (tsat < (NEEDLE_SHAFT_INTENSITY_P85_MIN << 8)) 
        tsat = (NEEDLE_SHAFT_INTENSITY_P85_MIN << 8);

    /* clamps in UQ8.8 */
    if (thr_low  > (250 << 8)) 
        thr_low  = (250 << 8);
    if (thr_high > (252 << 8)) 
        thr_high = (252 << 8);
    if (tsat > (255 << 8)) 
        tsat = (255 << 8); 

    if (thr_high <= thr_low) thr_high = thr_low + 1;
    if (tsat <= thr_high)    tsat = thr_high + 1;

    ev.thr_low  = (uint16_t)thr_low;
    ev.thr_high = (uint16_t)thr_high;
    ev.tsat     = (uint16_t)tsat;

    // calculate confidence evidence based on the local max values along the detected line in the rotated image within a band around the detected line (bandR)
    //hysteresis segments
    memset(kept_mask_local, 0, (size_t)len);

    int i = 0;
    while (i < len)
    {
        if (lm[i] < thr_low)
        {
            i++;
            continue;
        }

        int j = i;
        bool hasStrong = false;

        while (j < len && lm[j] >= thr_low)
        {
            if (lm[j] >= thr_high)
            {
                hasStrong = true;
            }
            j++;
        }

        if (hasStrong)
        {
            uint16_t run_len = (uint16_t)(j - i);
            memset(&kept_mask_local[i], 1, (size_t)run_len);
            ev.n_hit += run_len;
        }

        i = j;
    }

    // support
    ev.support = (float)ev.n_hit / (float)len;

     // gap-filled continuity
    fill_small_gaps(kept_mask_local, len, 1, kept_filled_local);
    {
        uint16_t run_len = longest_run_count(kept_filled_local, len);
        float runFrac = (float)run_len / (float)len;
        ev.longest_run = run_len;
        ev.continuity = 0.7f * runFrac + 0.3f * ev.support;
    }

    // strength
    uint32_t sum_norm = 0;
    uint16_t den = ev.tsat - ev.thr_high;
    if (den == 0)
        den = 1;

    for (int i = 0; i < len; ++i)
    {
        if (lm[i] <= ev.thr_high)
            LogDebug("do nothing");
        else if (lm[i] >= ev.tsat)
            sum_norm += Q15_ONE;
        else
            sum_norm += (uint32_t)((((uint64_t)(lm[i] - ev.thr_high)) << 15) + (den / 2u)) / den;
    }

    uint32_t avg = (sum_norm + len / 2u) / (uint32_t)len;
    if (avg > Q15_ONE) avg = Q15_ONE;
    ev.strength = (float)avg / (float)Q15_ONE;

    return ev;
}

// goertzel power calculation (efficient way to calculate discrete power spectrum)
static uint64_t goertzel_power(const int32_t *x, int N, int k)
{
    int64_t s0 = 0;
    int64_t s1 = 0;
    int64_t s2 = 0;
    int16_t coef = coef2cos_q14_128[k];

    for (int n = 0; n < REVERB_PSD_NFFT; ++n)
    {
        int64_t xn = (n < N) ? x[n] : 0;
        s0 = xn + ((int64_t)coef * s1 >> 14) - s2;
        s2 = s1;
        s1 = s0;
    }

    {
        int64_t a = (int64_t)s1 * (int64_t)s1;
        int64_t b = (int64_t)s2 * (int64_t)s2;
        int64_t c = ((int64_t)coef * (int64_t)s1 * (int64_t)s2) >> 14;
        int64_t p = a + b - c;

        if (p < 0)
            p = 0;

        return (uint64_t)p;
    }
}

// find maximum peak along detected needle line
static void find_max_peak(const uint64_t *psd, uint64_t *max_pk_val, uint16_t *max_pk_loc)
{
    // Initialize with 0 so any valid peak will replace it
    *max_pk_val = 0;
    *max_pk_loc = 0;

    for (uint16_t i = 1; i < (REVERB_PSD_BINS - 1); i++)
    {
        // checks if it's a local maximum (strictly > left, >= right)
        if (psd[i] > psd[i - 1] && psd[i] >= psd[i + 1])
        {
                
            //Keep only the biggest one found so far
            if (psd[i] > *max_pk_val)
            {
                *max_pk_val = psd[i];
                *max_pk_loc = i;
            }
        }
    }

    // fallback if no local peak was found
   /* if (*max_pk_val == 0)
    {
        for (uint16_t i = 0; i < REVERB_PSD_BINS; ++i)
        {
            if (psd[i] > *max_pk_val)
            {
                *max_pk_val = psd[i];
                *max_pk_loc = i;
            }
        }
    }*/
}

// calculate reverberation score beneath detedted needle line
static void needle_reverb_score( const float *Irot_best,  int Hrot_best,  int Wrot_best,  int stride_rot,  NeedleLineResult best_needle_line, float *reverb_ratio, float *reverb_score)
{
    int y0 = clampi(best_needle_line.best_rho, 0, Hrot_best - 1);
    int x0 = best_needle_line.tip_x;
    int x1 = best_needle_line.entry_x;

    int32_t r[REVERB_OFFLINE * 2 + 1];
    uint64_t mean_psd[REVERB_PSD_BINS] = {0};
    uint16_t vals[REVERB_OFFLINE * 2 + 1];

    *reverb_ratio = 0.0f;
    *reverb_score = 0.0f;

    if (x0 > x1)
    {
        int t = x0;
        x0 = x1;
        x1 = t;
    }

    int dx = x1 - x0; 
    int xs[REVERB_LINE_COUNT];
    int num_xs = 0;

    if (dx < REVERB_LINE_COUNT)
    {
        num_xs = dx + 1;
        for (int i = 0; i < num_xs; ++i)
            xs[i] = x0 + i;
    }
    else
    {
        int pad = (int)roundf(0.25f * (float)dx);
        if (pad < 2) pad = 2;

        if (dx < (2 * pad + REVERB_LINE_COUNT))
            pad = (dx - REVERB_LINE_COUNT) / 2;

        if (pad < 0) pad = 0;

        int xa = x0 + pad;
        int xb = x1 - pad;

        for (int i = 0; i < REVERB_LINE_COUNT; ++i)
        {
            xs[i] = xa + (int)((int64_t)i * dx / (REVERB_LINE_COUNT - 1));
            xs[i] = clampi(xs[i], x0, x1);
        }

        num_xs = REVERB_LINE_COUNT;
    }

    int yA = clampi(y0 - REVERB_OFFLINE, 0, Hrot_best - 1);
    int yB = clampi(y0 + REVERB_OFFLINE, 0, Hrot_best - 1);
    int N = yB - yA + 1;

    if (N <= 0 || N > (REVERB_OFFLINE * 2 + 1))
        return;

    for (int m = 0; m < num_xs; ++m)
    {
        int x = xs[m];
        uint32_t base = (uint32_t)x * (uint32_t)stride_rot;

        uint16_t vals[N];
        uint32_t sum = 0;

        for (int i = 0; i < N; ++i)
        {
            vals[i] = float_to_ushort(Irot_best[base + (uint32_t)(yA + i)]);
            sum += vals[i];
        }

        uint16_t mean = (uint16_t)((sum + N / 2u) / (uint32_t)N);

        for (int i = 0; i < N; ++i)
        {
            int32_t dc_removed = (int32_t)vals[i] - (int32_t)mean;
            r[i] =(int32_t)((((int64_t)dc_removed * hamming41_q15[i]) + (1 << 14)) >> 15);
        }

        for (int k = 0; k < REVERB_PSD_BINS; ++k)
            mean_psd[k] += goertzel_power(r, N, k);
           
    }

    uint32_t inv_line_count = (uint32_t)((((uint64_t)1 << 16) + (num_xs / 2)) / (uint64_t)num_xs); // rounded version of reciprocal to avoid precision loss, using 64-bit intermediate to prevent overflow when num_xs is large, and adding (num_xs / 2) for rounding before shifting right by 16
    for (int k = 0; k < REVERB_PSD_BINS; ++k)
    {
        // multiply by the inverse and shift right by 16, adding (1 << 15) for rounding
        mean_psd[k] = (uint64_t)((mean_psd[k] * inv_line_count + (1U << 15)) >> 16);
    }

    // find the maximum peak in the mean_psd array using the same logic as find_first_peak_64bit, but keeping track of the maximum value and its location.
    uint64_t max_pk_val;
    uint16_t max_pk_loc;
    find_max_peak(mean_psd, &max_pk_val, &max_pk_loc);
    if (max_pk_val == 0u)
        return;

    // sort the mean_psd array in descending order using insertion sort (since REVERB_PSD_BINS is small), while keeping the values as 64-bit integers to avoid overflow issues.
    for (int i = 1; i < REVERB_PSD_BINS; ++i)
    {
        uint64_t key = mean_psd[i];
        int j = i - 1;

        while (j >= 0 && mean_psd[j] < key)
        {
            mean_psd[j + 1] = mean_psd[j];
            --j;
        }

        mean_psd[j + 1] = key;
    }

    uint64_t noise_sum = 0;
    int noise_count = 0;

    for (int i = 20; i < REVERB_PSD_BINS - 3; ++i)
    {
        noise_sum += mean_psd[i];
        noise_count++;
    }

    uint64_t noise_floor = (noise_count > 0) ? (noise_sum / (uint64_t)noise_count) : 1u;
    if (noise_floor == 0)
        return;
        //noise_floor = 1u;

    uint32_t curr_ratio = (uint32_t)((max_pk_val << 8) / noise_floor);
    uint32_t smooth_ratio = 0;
    uint32_t SNRratioWeightsSum = 0;

    for(int i = 0; i < MAX_NEEDLE_FRAMES; ++i)
    {
        int logical_idx = buffer_head + i; // now buffer_head points to the oldest frame in the circular buffer before new frame data is stored
        if (logical_idx >= MAX_NEEDLE_FRAMES)
            logical_idx -= MAX_NEEDLE_FRAMES;
        if(frame_buffer_multiFrame_idx[logical_idx] == -1) // if there is no valid SNRration for previous frame stored in the circular buffer at the expected index
            oldSNRratios[logical_idx] = curr_ratio; // store the current SNR ratio in the oldSNRratios array for use in the next frames
        smooth_ratio += multi_frame_Weights[i] * oldSNRratios[logical_idx]; // accumulate the weighted SNR ratios for the current frame and previous frames
        SNRratioWeightsSum += multi_frame_Weights[i];
    }

    // calculate inverse: (1 << 16) / total_weight
    uint32_t inv_total_weight = (uint32_t)((((uint64_t)1 << 16) + (SNRratioWeightsSum / 2)) / (uint64_t)SNRratioWeightsSum); // rounded version of reciprocal to avoid precision loss, using 64-bit intermediate to prevent overflow when SNRratioWeightsSum is large, and adding (SNRratioWeightsSum / 2) for rounding before shifting right by 16
    // Use 64-bit to prevent (weighted_sum * inv_total_weight) from overflowing 32-bit. Add (1 << 15) before shifting to achieve nearest-neighbor rounding
    smooth_ratio = (uint32_t)(((uint64_t)smooth_ratio * inv_total_weight + (1u << 15)) >> 16);
    
    *reverb_ratio = (float)smooth_ratio / 256.0f; // convert back to float from Q8.8
    *reverb_score = *reverb_ratio / 100.0f; // normalize to [0, 1] range (assuming 100.0f is the max expected ratio for normalization, cannot garantee the max ratio will be 100.0f, this is just an example, the actual normalization factor may need to be determined empirically based on the expected range of SNR ratios in real scenarios)
}

// calculate the confidence score for the detected needle line in a single frame based on the evidence from the local max values along the detected line in the rotated image and the reverberation score, where the input image is in float format and the output confidence score is in Q15 fixed point format.
NeedleConfidenceSingleFrame needle_line_confidence_singleFrame(float *Irot_best, int Hrot_best, int Wrot_best, int stride_rot, NeedleLineResult *best_needle_line)    
{      
    NeedleConfidenceSingleFrame conf = {0};
    NeedleLineEvidence centerEv = {0};
    float conf_base_sum;
    float score = 0;
    float ratio = 0;

    int bandR = BAND_R;
    if (bandR <= 0)
        bandR = 1;

    centerEv = needle_line_confidence(Irot_best, Hrot_best, Wrot_best, stride_rot, *best_needle_line, bandR);

    float centerEnergy = centerEv.support * centerEv.strength;

    if (centerEv.n_total < MIN_VALID_NEEDLE_SIZE_PIXELS || centerEnergy <= 0)
    {
        centerEv.reverb_score = 0.0;
        centerEv.reverb_ratio = 0.0;
        conf.confidence_score_singleFrame = 0.0;
        conf.centerEv = centerEv;
        return conf;
    }

    conf_base_sum = centerEv.support + centerEv.strength + centerEv.continuity;

    if (conf_base_sum < CONF_BASE_THRESH)
    {
        centerEv.reverb_score = conf_base_sum;  
        centerEv.reverb_ratio = 0.0;
        conf.confidence_score_singleFrame = conf_base_sum;
        conf.centerEv = centerEv;
        return conf;
    }

    needle_reverb_score(Irot_best, Hrot_best, Wrot_best, stride_rot, *best_needle_line, &ratio, &score);

    centerEv.reverb_ratio = ratio;
    centerEv.reverb_score = score > 1.0f ? 1.0f : score;

    float confidence_score_base = 0.33f * centerEv.support + 0.34f * centerEv.strength + 0.33f * centerEv.continuity;

    /*if(needleMultiFrameState.tracked == 0)
        conf.confidence_score_singleFrame = confidence_score_base * ( 0.2 + 0.8 * centerEv.reverb_score);
    else
        conf.confidence_score_singleFrame = confidence_score_base * ( 0.5 + 0.5 * centerEv.reverb_score);*/
    
    conf.confidence_score_singleFrame = confidence_score_base * ( 0.7 + 0.3 * centerEv.reverb_score);
    conf.centerEv = centerEv;

    return conf;
}

// apply STA/LTA and z-score normalization to the multi-frame confidence score for better temporal stability and consistency
static inline void needle_logic_reset(NeedleLogicState *nlst, float current_conf_score)
{
    if (nlst == NULL) return;

    nlst->initialized = 1U;
    nlst->no_need_to_reset = true;

    nlst->sta = current_conf_score;
    nlst->lta_on = current_conf_score;
    nlst->lta_off = current_conf_score;

    nlst->ratio_mean_off = 1.0f;
    nlst->ratio_var_off  = 0.0025f; // 0.05^2, can be tuned based on expected variability of the ratio in the OFF state

    nlst->state = NEEDLE_STATE_OFF;

    nlst->arm_count = 0;
    nlst->confirm_count = 0;
    nlst->decay_count = 0;
    nlst->off_count = 0;

    nlst->peak_sta_on = 0.0f;
    nlst->peak_conf_on = 0.0f;
}
 
/* update_needle_logic_live() is a live state machine that decides when needle enhancement should be off or on.
It processes one confidence score per frame and maintains a short-term average (STA) and a long-term average (LTA). 
The STA reacts quickly to recent confidence changes, while the LTA tracks the slower background trend. 
Their ratio helps detect a meaningful rise above baseline.
The function has four states, two of them are transitional states (on to off, off to on)
OFF: enhancement is disabled.
ARMED: a possible needle onset has been detected, but it is not stable enough yet.
ON: needle detection is considered stable, so enhancement is fully enabled.
DECAY: the needle signal is weakening, so enhancement is reduced but not fully removed yet.
While the function is in the OFF state, it learns the normal baseline behavior of the STA/LTA ratio. 
From that it computes a z-score, which measures how unusual the current ratio is compared with normal OFF-state behavior. 
The state transitions are:
OFF to ARMED: triggered when the ratio rises clearly above baseline and the z-score is high enough for a small number of frames.
ARMED to ON: triggered only if the signal then reaches and maintains a stable plateau, meaning STA is high enough, 
LTA is also high enough, and the ratio remains slightly above baseline for several consecutive frames.
ON to DECAY: triggered when the short-term evidence weakens, meaning STA drops and either the ratio or z-score also weakens.
DECAY to ON: triggered if the signal recovers and again looks stable.
DECAY to OFF: triggered if weak evidence continues for several frames.
The function returns a binary enhancement decision. The binary decision
indicates whether enhancement should be active.*/

static bool update_needle_logic_live(float current_conf_score, NeedleLogicState *nlst, NeedleLogicDebug *dbg)
{
    float ratio;
    float d;
    float var_eff;
    float diff;
    float diff2;
    bool enhancement_on;

    if (nlst == NULL)
        return false;

    if ((!nlst->initialized) || !nlst->no_need_to_reset)
        needle_logic_reset(nlst, current_conf_score);

    // update STA/LTA
    float alphaStaEff = virtual_gate_scaled_alpha(kNeedleLogicParams.alpha_sta);
    float alphaLtaOnEff = virtual_gate_scaled_alpha(kNeedleLogicAlphaLtaOn);
    float alphaLtaOffEff = virtual_gate_scaled_alpha(kNeedleLogicParams.alpha_lta);
    nlst->sta = (1.0f - alphaStaEff) * nlst->sta + alphaStaEff * current_conf_score;
    nlst->lta_on = (1.0f - alphaLtaOnEff) * nlst->lta_on + alphaLtaOnEff * current_conf_score;
    nlst->lta_off = (1.0f - alphaLtaOffEff) * nlst->lta_off + alphaLtaOffEff * current_conf_score;

    ratio = nlst->sta / fmaxf(nlst->lta_off, kNeedleLogicParams.lta_floor);

    // update OFF-state ratio baseline only while OFF
    if (nlst->state == NEEDLE_STATE_OFF && nlst->lta_off < cfg.T_off)
    {
        d = ratio - nlst->ratio_mean_off;
        float betaRatioOffEff = virtual_gate_scaled_alpha(kNeedleLogicParams.beta_ratio_off);
        nlst->ratio_mean_off = nlst->ratio_mean_off + betaRatioOffEff * d;
        nlst->ratio_var_off = (1.0f - betaRatioOffEff) * nlst->ratio_var_off + betaRatioOffEff * d * d;
    }

    var_eff = fmaxf(nlst->ratio_var_off, kNeedleLogicParams.sigma_ratio_min * kNeedleLogicParams.sigma_ratio_min);
    diff = ratio - nlst->ratio_mean_off;
    diff2 = diff * diff;

    switch (nlst->state)
    {
        case NEEDLE_STATE_OFF:
        {
            bool onset_cond;
            bool presence_cond;

            onset_cond = (ratio > kNeedleLogicParams.r_arm) && (diff > 0.0f) && (diff2 > (kNeedleLogicParams.z_arm * kNeedleLogicParams.z_arm) * var_eff);
            presence_cond = (nlst->sta > kNeedleLogicParams.sta_on) && (nlst->lta_on > kNeedleLogicParams.lta_on);

            if (onset_cond || presence_cond)
                nlst->arm_count++;
            else
                nlst->arm_count = 0;

            if (nlst->arm_count >= virtual_gate_fast_on_count(kNeedleLogicParams.k_arm))
            {
                nlst->state = NEEDLE_STATE_ARMED;
                nlst->arm_count = 0;
                nlst->confirm_count = 0;
            }

            break;
        }
       case NEEDLE_STATE_ARMED:
        {
            bool confirm_cond;
            bool neg_z_disarm;

            confirm_cond = (nlst->sta > kNeedleLogicParams.sta_on) && (nlst->lta_on > kNeedleLogicParams.lta_on);

            if (confirm_cond)
                nlst->confirm_count++;
            else
            {
                neg_z_disarm = (diff < 0.0f) && (diff2 > (0.5f * 0.5f) * var_eff);

                if ((nlst->sta < kNeedleLogicParams.sta_off) || ((ratio < 1.0f) && neg_z_disarm))
                {
                    nlst->state = NEEDLE_STATE_OFF;
                    nlst->confirm_count = 0;
                    nlst->arm_count = 0;
                }
                else
                {
                    if (nlst->confirm_count > 0)
                        nlst->confirm_count--;
                }
            }

            if (nlst->confirm_count >= virtual_gate_fast_on_count(kNeedleLogicParams.k_confirm))
            {
                nlst->state = NEEDLE_STATE_ON;
                nlst->confirm_count = 0;
                nlst->decay_count = 0;
                nlst->off_count = 0;
                nlst->peak_sta_on = nlst->sta;
                nlst->peak_conf_on = current_conf_score;
            }

            break;
        }

        case NEEDLE_STATE_ON:
        {
            nlst->peak_sta_on = fmaxf(nlst->peak_sta_on, nlst->sta);
            nlst->peak_conf_on = fmaxf(nlst->peak_conf_on, current_conf_score);

            if ((nlst->sta < kNeedleLogicParams.sta_decay) && ((ratio < kNeedleLogicParams.r_decay) || ((diff < 0.0f) && (diff2 > (kNeedleLogicParams.z_decay * kNeedleLogicParams.z_decay) * var_eff))))
                nlst->decay_count++;
            else
                nlst->decay_count = 0;

            if (nlst->decay_count >= virtual_gate_slow_off_count(kNeedleLogicParams.k_decay))
            {
                nlst->state = NEEDLE_STATE_DECAY;
                nlst->decay_count = 0;
                nlst->off_count = 0;
            }
            break;
        }

        case NEEDLE_STATE_DECAY:
        default:
        {
            if ((nlst->sta > kNeedleLogicParams.sta_on) && (nlst->lta_on > kNeedleLogicParams.lta_on)) // && (ratio > kNeedleLogicParams.r_confirm))
            {
                nlst->state = NEEDLE_STATE_ON;
                nlst->off_count = 0;
                nlst->decay_count = 0;
            }
            else
            {
                if ((nlst->sta < kNeedleLogicParams.sta_off) && (ratio < kNeedleLogicParams.r_off) && (diff < 0.0f) && (diff2 > (kNeedleLogicParams.z_off * kNeedleLogicParams.z_off) * var_eff))
                    nlst->off_count++;

                else
                    nlst->off_count = 0;

                if (nlst->off_count >= virtual_gate_slow_off_count(kNeedleLogicParams.k_off))
                {
                    nlst->state = NEEDLE_STATE_OFF;
                    nlst->off_count = 0;
                    nlst->arm_count = 0;
                    nlst->confirm_count = 0;
                    nlst->decay_count = 0;
                    nlst->ratio_mean_off = ratio;
                    nlst->ratio_var_off = kNeedleLogicParams.sigma_ratio_min * kNeedleLogicParams.sigma_ratio_min;
                }
            }
            break;
        }
    }

    enhancement_on = ((nlst->state == NEEDLE_STATE_ON) || (nlst->state == NEEDLE_STATE_DECAY));

    if (dbg != NULL)
    {
        dbg->sta = nlst->sta;
        dbg->lta = nlst->lta_off;
        dbg->ratio = ratio;
        dbg->ratio_mean_off = nlst->ratio_mean_off;

        dbg->state = (int)nlst->state;
        dbg->arm_count = nlst->arm_count;
        dbg->confirm_count = nlst->confirm_count;
        dbg->decay_count = nlst->decay_count;
        dbg->off_count = nlst->off_count;

        dbg->STA_on = kNeedleLogicParams.sta_on;
        dbg->LTA_on = kNeedleLogicParams.lta_on;
        dbg->STA_decay = kNeedleLogicParams.sta_decay;
        dbg->STA_off = kNeedleLogicParams.sta_off;
        dbg->R_arm = kNeedleLogicParams.r_arm;
        dbg->R_confirm = kNeedleLogicParams.r_confirm;
        dbg->R_decay = kNeedleLogicParams.r_decay;
        dbg->R_off = kNeedleLogicParams.r_off;
        dbg->Z_arm = kNeedleLogicParams.z_arm;
        dbg->Z_decay = kNeedleLogicParams.z_decay;
        dbg->Z_off = kNeedleLogicParams.z_off;

        dbg->peak_sta_on = nlst->peak_sta_on;
        dbg->peak_conf_on = nlst->peak_conf_on;
    }

    return enhancement_on;
}

// track needle status with multiple frames
void needle_line_confidence_multiFrames(float conf_frame, NeedleLineResult best_needle_line_singleFrame)
{
    if (!needleMultiFrameState.initialized)
    {
        needleMultiFrameState.initialized = 1;
        needleMultiFrameState.tracked = 0;
        needleMultiFrameState.onCount = 0;
        needleMultiFrameState.offCount = 0;
        needleMultiFrameState.confSmooth = conf_frame;
        needleMultiFrameState.smoothRho = best_needle_line_singleFrame.best_rho;
        needleMultiFrameState.smoothTheta_deg = best_needle_line_singleFrame.theta_deg;
        needleMultiFrameState.smoothTip_x = (float)best_needle_line_singleFrame.tip_x;
    }

    // stable line tracker check based on the distance and angle difference between the detected line in the current frame and the smoothed line parameters from previous frames, where the smoothing is done by exponential moving average (EMA) to give more weight to recent frames while still considering the historical trend, and the confidence score for the current frame is also taken into account to decide whether to update the smoothed line parameters with a slower adaptation rate when there is a large jump in line parameters but the confidence score is still high, which can help maintain stability in needle tracking while allowing for some flexibility in handling sudden changes in needle position or orientation.
    int32_t dist_err = best_needle_line_singleFrame.best_rho - needleMultiFrameState.smoothRho;
    float ang_err  = best_needle_line_singleFrame.theta_deg - needleMultiFrameState.smoothTheta_deg;
    
    // only update if the detection is strong and close to the smoothed line parameters, otherwise keep the smoothed line parameters unchanged to maintain stability in needle tracking
    if (conf_frame > cfg.confidence_thr && abs(dist_err) < cfg.max_rho_jump && fabsf(ang_err) < cfg.max_theta_deg_jump)
    {
        needleMultiFrameState.smoothRho = cfg.emaAlpha * (best_needle_line_singleFrame.best_rho - needleMultiFrameState.smoothRho) + needleMultiFrameState.smoothRho;
        needleMultiFrameState.smoothTheta_deg = cfg.emaAlpha * (best_needle_line_singleFrame.theta_deg - needleMultiFrameState.smoothTheta_deg) + needleMultiFrameState.smoothTheta_deg;
        needleMultiFrameState.smoothTip_x = cfg.emaAlpha * ((float)best_needle_line_singleFrame.tip_x - needleMultiFrameState.smoothTip_x) + needleMultiFrameState.smoothTip_x;
    }
    else if (needleMultiFrameState.confSmooth >= cfg.confidence_thr && conf_frame <= cfg.confidence_thr && needleMultiFrameState.tracked)
        LogDebug("do nothing");
    else
    {
        needleMultiFrameState.smoothRho = best_needle_line_singleFrame.best_rho;
        needleMultiFrameState.smoothTheta_deg = best_needle_line_singleFrame.theta_deg;
        needleMultiFrameState.smoothTip_x = (float)best_needle_line_singleFrame.tip_x;
    }

    needleMultiFrameState.confSmooth = virtual_gate_scaled_alpha(cfg.emaAlpha) * (conf_frame - needleMultiFrameState.confSmooth) + needleMultiFrameState.confSmooth;

    if (!needleMultiFrameState.tracked)
    {
        if (needleMultiFrameState.confSmooth >= cfg.T_on)
            needleMultiFrameState.onCount = (needleMultiFrameState.onCount < 255) ? needleMultiFrameState.onCount + 1 : 255;
        else
            needleMultiFrameState.onCount = 0;

        if (needleMultiFrameState.onCount >= virtual_gate_fast_on_count(cfg.K_on))
        {
            needleMultiFrameState.tracked = 1;
            needleMultiFrameState.offCount = 0;
        }
    }
    else
    {
        if (needleMultiFrameState.confSmooth < cfg.T_off)
            needleMultiFrameState.offCount = (needleMultiFrameState.offCount < 255) ? needleMultiFrameState.offCount + 1 : 255;
        else
            needleMultiFrameState.offCount = 0;

        if (needleMultiFrameState.offCount >= virtual_gate_slow_off_count(cfg.K_off))
        {
            needleMultiFrameState.tracked = 0;
            needleMultiFrameState.onCount = 0;
        }
    }

    //needleMultiFrameState.enhancementLogicOn = update_needle_logic_live(needleMultiFrameState.confSmooth, &needleLogicState, NULL);
    needleMultiFrameState.enhancementLogicOn = update_needle_logic_live(conf_frame, &needleLogicState, NULL);

    if (needleMultiFrameState.enhancementLogicOn && needleMultiFrameState.tracked)
        needleMultiFrameState.enhancementActive = true;
    else
        needleMultiFrameState.enhancementActive = false;
}
