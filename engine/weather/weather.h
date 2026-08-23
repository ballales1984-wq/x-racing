#pragma once

#include "common.h"

// Project 0 — weather model
// Namespace: p0::weather
namespace p0::weather {

// Current weather conditions
struct WeatherState {
  double rain_intensity = 0.0;            // [0, 1], 0=dry, 1=heavy rain
  double wind_speed = 0.0;                // m/s
  double wind_direction = 0.0;            // rad, wind direction
  double air_temperature = 300.0;         // K, air temperature
  double humidity = 0.5;                  // [0, 1], relative humidity
  double track_temperature = 305.0;       // K, track surface temperature
};

// Weather parameters: how weather affects the simulation
struct WeatherParams {
  double rain_grip_reduction = 0.4;       // grip reduction factor in rain [0, 1]
  double rain_rolling_resistance = 1.3;   // rolling resistance multiplier in rain
  double wind_effect_on_speed = 0.1;      // wind effect on vehicle speed
  double temp_cooling_rate = 0.2;         // K/s per K of temperature difference
  double rain_cooling = 2.0;              // K/s cooling from rain
  double track_heat_rate = 0.05;          // K/s track heating from sun
};

}
