#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ch;
    bool sure;
    bool found;
    int confidence_pct;
} inference_result_t;

void inference_run(const uint8_t *canvas, int canvas_w, int canvas_h, int ui_top_margin,
                   inference_result_t *out_result);
size_t inference_workspace_bytes(void);

#ifdef __cplusplus
}
#endif
