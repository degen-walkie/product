/* Hold-to-talk capture on the ES8388 and speech-to-text through the relay. */
#pragma once
#include <stdbool.h>

void audio_init(void);   /* after board_init(); logs and disables voice if the codec is missing */
void audio_start(void);
bool audio_stop(void);   /* true if a clip is on its way; its text arrives via dw_app_voice_result() */
