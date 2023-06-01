#pragma once

typedef struct {
  int adv_period;
  int power;
  int hit_threshold;
  int fall_threshold;
} app_settings_s;

extern app_settings_s app_settings;