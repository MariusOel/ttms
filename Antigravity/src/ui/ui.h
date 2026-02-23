#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void ui_init(void);
void ui_update_data(float front_temp, float rear_temp);

#ifdef __cplusplus
}
#endif

#endif
