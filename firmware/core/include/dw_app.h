#pragma once
#include "dw_hal.h"
#include "lvgl.h"

#define DW_FW_VERSION "0.1.0"

/* Call once after LVGL and the display/input drivers are up, from the UI thread. */
void dw_app_init(void);
