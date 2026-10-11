#pragma once

/* NVS, Wi-Fi, SNTP and the status light. */
void platform_init(void);

/* Run fn(arg) on the UI task (the only task that touches LVGL and the core). */
void platform_post(void (*fn)(void *), void *arg);
