/**
 * Configuration_adv.h
 * Minimal, safe and commonly used advanced Marlin settings.
 * Replace your existing Configuration_adv.h with this file.
 */

#pragma once

#define CONFIGURATION_ADV_H_VERSION 02010300

// Configuration export
#define CONFIG_EXPORT 1 // 1: JSON

// Thermal settings
#define THERMOCOUPLE_MAX_ERRORS 5

// Custom thermistor example for sensor id 1000
#if TEMP_SENSOR_0 == 1000
  #define HOTEND0_PULLUP_RESISTOR_OHMS    4700
  #define HOTEND0_RESISTANCE_25C_OHMS   100000
  #define HOTEND0_BETA                    3950
  #define HOTEND0_SH_C_COEFF                 0
#endif

#if TEMP_SENSOR_BED == 1000
  #define BED_PULLUP_RESISTOR_OHMS        4700
  #define BED_RESISTANCE_25C_OHMS       100000
  #define BED_BETA                        3950
  #define BED_SH_C_COEFF                     0
#endif

// Heated bed bang-bang safe defaults (if PID disabled)
#if DISABLED(PIDTEMPBED)
  #define BED_CHECK_INTERVAL 2000   // ms
  #if ANY(BED_LIMIT_SWITCHING, PELTIER_BED)
    #define BED_HYSTERESIS 2
  #endif
#endif

// Thermal protection (hotends)
#if ALL(HAS_HOTEND, THERMAL_PROTECTION_HOTENDS)
  #define THERMAL_PROTECTION_PERIOD        40
  #define THERMAL_PROTECTION_HYSTERESIS     4
  #define WATCH_TEMP_PERIOD  40
  #define WATCH_TEMP_INCREASE 2
#endif

// Thermal protection (bed)
#if TEMP_SENSOR_BED && ENABLED(THERMAL_PROTECTION_BED)
  #define THERMAL_PROTECTION_BED_PERIOD        240
  #define THERMAL_PROTECTION_BED_HYSTERESIS     3
  #define WATCH_BED_TEMP_PERIOD                180
  #define WATCH_BED_TEMP_INCREASE               2
#endif

// Variance monitor disabled by default (experimental)
#undef THERMAL_PROTECTION_VARIANCE_MONITOR

// PID options
#if ENABLED(PIDTEMP)
  //#define PID_EXTRUSION_SCALING
  //#define PID_FAN_SCALING
  #if ENABLED(PID_PARAMS_PER_HOTEND)
    #define DEFAULT_KF_LIST { 10, 10 }
  #endif
#endif

// Autotemp (safe defaults)
#define AUTOTEMP
#if ENABLED(AUTOTEMP)
  #define AUTOTEMP_OLDWEIGHT    0.98
  #define AUTOTEMP_MIN        210
  #define AUTOTEMP_MAX        250
  #define AUTOTEMP_FACTOR       0.1f
#endif

// Fans
#define FAN_MIN_PWM 50
//#define FAN_MAX_PWM 128

// Controller fan (disabled by default)
#undef USE_CONTROLLER_FAN

// Fan kickstart (disabled by default)
//#define FAN_KICKSTART_TIME  100
//#define FAN_KICKSTART_POWER 180

// Extruder runout prevention (disabled by default)
#undef EXTRUDER_RUNOUT_PREVENT

// Hotend idle timeout (disabled by default)
#undef HOTEND_IDLE_TIMEOUT

// AD595/AD8495 calibration defaults
#define TEMP_SENSOR_AD595_OFFSET  0.0
#define TEMP_SENSOR_AD595_GAIN    1.0
#define TEMP_SENSOR_AD8495_OFFSET 0.0
#define TEMP_SENSOR_AD8495_GAIN   1.0

// Laser cooler defaults (disabled unless TEMP_SENSOR_COOLER set)
#undef TEMP_SENSOR_COOLER

// Board sensor defaults (disabled unless TEMP_SENSOR_BOARD set)
#undef TEMP_SENSOR_BOARD

// SoC sensor defaults (disabled unless TEMP_SENSOR_SOC set)
#undef TEMP_SENSOR_SOC

// Safety: do not enable experimental features by default
#undef THERMAL_PROTECTION_VARIANCE_MONITOR

// End of minimal Configuration_adv.h
