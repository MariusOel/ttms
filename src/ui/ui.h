#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void ui_init(void);
void ui_show_splash(const char *version, const char *ip);
void ui_update_data(float front_temp, float rear_temp, bool refresh);

#ifdef __cplusplus
}
#endif

#endif
