#include "inference.h"

#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "model_data.h"

static float buf_conv1[8 * 28 * 28];
static float buf_pool1[8 * 14 * 14];
static float buf_conv2[16 * 14 * 14];
static float buf_pool2[16 * 7 * 7];
static float buf_fc1[64];
static float buf_out[62];

static void conv2d(const float *in, float *out, int ic, int oc, int h, int w,
                   const int8_t *weight, const float *bias, float scale) {
    for (int o = 0; o < oc; o++) {
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                float val = 0;
                for (int c = 0; c < ic; c++) {
                    for (int ki = -1; ki <= 1; ki++) {
                        for (int kj = -1; kj <= 1; kj++) {
                            int ni = i + ki, nj = j + kj;
                            if (ni >= 0 && ni < h && nj >= 0 && nj < w) {
                                val += in[c * h * w + ni * w + nj] *
                                       (weight[o * ic * 9 + c * 9 + (ki + 1) * 3 + (kj + 1)] *
                                        scale);
                            }
                        }
                    }
                }
                out[o * h * w + i * w + j] = val + bias[o];
            }
        }
    }
}

static void relu(float *d, int s) {
    for (int i = 0; i < s; i++) {
        if (d[i] < 0) d[i] = 0;
    }
}

static void maxpool(const float *in, float *out, int c, int h, int w) {
    for (int ch = 0; ch < c; ch++) {
        for (int i = 0; i < h / 2; i++) {
            for (int j = 0; j < w / 2; j++) {
                float mv = -1e10f;
                for (int ki = 0; ki < 2; ki++) {
                    for (int kj = 0; kj < 2; kj++) {
                        float v = in[ch * h * w + (i * 2 + ki) * w + (j * 2 + kj)];
                        if (v > mv) mv = v;
                    }
                }
                out[ch * (h / 2) * (w / 2) + i * (w / 2) + j] = mv;
            }
        }
    }
}

static void fc_layer(const float *in, float *out, int id, int od, const int8_t *weight,
                     const float *bias, float scale) {
    for (int o = 0; o < od; o++) {
        float val = 0;
        for (int i = 0; i < id; i++) {
            val += in[i] * (weight[o * id + i] * scale);
        }
        out[o] = val + bias[o];
    }
}

void inference_run(const uint8_t *canvas, int canvas_w, int canvas_h, int ui_top_margin,
                   inference_result_t *out_result) {
    int x0 = canvas_w, x1 = 0, y0 = canvas_h, y1 = 0;
    bool found = false;
    for (int y = ui_top_margin; y < canvas_h; y++) {
        for (int x = 0; x < canvas_w; x++) {
            if (canvas[y * canvas_w + x]) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
                found = true;
            }
        }
    }

    if (!found) {
        out_result->ch = '?';
        out_result->sure = false;
        out_result->found = false;
        out_result->confidence_pct = 0;
        return;
    }

    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    int size = (w > h) ? w : h;

    float downsampled[784] = {0};
    for (int i = 0; i < 28; i++) {
        for (int j = 0; j < 28; j++) {
            int sx = x0 + (j * size) / 28 - (size - w) / 2;
            int sy = y0 + (i * size) / 28 - (size - h) / 2;

            if (sx >= 0 && sx < canvas_w && sy >= ui_top_margin && sy < canvas_h) {
                if (canvas[sy * canvas_w + sx]) {
                    downsampled[j * 28 + i] = 1.0f;
                }
            }
        }
    }

    conv2d(downsampled, buf_conv1, 1, 8, 28, 28, conv1_weight, conv1_bias, conv1_scale);
    relu(buf_conv1, 8 * 28 * 28);
    maxpool(buf_conv1, buf_pool1, 8, 28, 28);
    conv2d(buf_pool1, buf_conv2, 8, 16, 14, 14, conv2_weight, conv2_bias, conv2_scale);
    relu(buf_conv2, 16 * 14 * 14);
    maxpool(buf_conv2, buf_pool2, 16, 14, 14);
    fc_layer(buf_pool2, buf_fc1, 784, 64, fc1_weight, fc1_bias, fc1_scale);
    relu(buf_fc1, 64);
    fc_layer(buf_fc1, buf_out, 64, 62, fc2_weight, fc2_bias, fc2_scale);

    int pred = 0;
    float ms = buf_out[0];
    float second = buf_out[0];
    for (int i = 1; i < 62; i++) {
        if (buf_out[i] > ms) {
            second = ms;
            ms = buf_out[i];
            pred = i;
        } else if (buf_out[i] > second) {
            second = buf_out[i];
        }
    }

    const char *chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    float sum = 0.0f;
    for (int i = 0; i < 62; i++) {
        sum += expf(buf_out[i] - ms);
    }
    float confidence = (sum > 0.0f) ? (100.0f / sum) : 0.0f;
    if (confidence < 0.0f) confidence = 0.0f;
    if (confidence > 99.0f) confidence = 99.0f;

    out_result->ch = chars[pred];
    out_result->sure = ((ms - second) > 1.0f);
    out_result->found = true;
    out_result->confidence_pct = (int)(confidence + 0.5f);
}

size_t inference_workspace_bytes(void) {
    return sizeof(buf_conv1) + sizeof(buf_pool1) + sizeof(buf_conv2) + sizeof(buf_pool2) +
           sizeof(buf_fc1) + sizeof(buf_out);
}
