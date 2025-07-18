/**
 * @file vl53l0x.h
 * @brief Define a interface pública (API) para o driver do sensor de distância ToF VL53L0X.
 *
 * Este arquivo declara as estruturas e funções para interagir com o sensor,
 * abstraindo a complexa sequência de inicialização e leitura.
 */

/**
 * https://github.com/ASCCJR
 */

#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

#define VL53L0X_ADDRESS 0x29

/**
 * @brief Enumeração completa dos registradores do VL53L0X.
 * Usar nomes em vez de números hexadecimais torna o código do driver mais legível.
 */
enum regAddr {
  SYSRANGE_START                              = 0x00,
  SYSTEM_THRESH_HIGH                          = 0x0C,
  SYSTEM_THRESH_LOW                           = 0x0E,
  SYSTEM_SEQUENCE_CONFIG                      = 0x01,
  SYSTEM_RANGE_CONFIG                         = 0x09,
  SYSTEM_INTERMEASUREMENT_PERIOD              = 0x04,
  SYSTEM_INTERRUPT_CONFIG_GPIO                = 0x0A,
  GPIO_HV_MUX_ACTIVE_HIGH                     = 0x84,
  SYSTEM_INTERRUPT_CLEAR                      = 0x0B,
  RESULT_INTERRUPT_STATUS                     = 0x13,
  RESULT_RANGE_STATUS                         = 0x14,
  RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN       = 0xBC,
  RESULT_CORE_RANGING_TOTAL_EVENTS_RTN        = 0xC0,
  RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF       = 0xD0,
  RESULT_CORE_RANGING_TOTAL_EVENTS_REF        = 0xD4,
  RESULT_PEAK_SIGNAL_RATE_REF                 = 0xB6,
  ALGO_PART_TO_PART_RANGE_OFFSET_MM           = 0x28,
  I2C_SLAVE_DEVICE_ADDRESS                    = 0x8A,
  MSRC_CONFIG_CONTROL                         = 0x60,
  PRE_RANGE_CONFIG_MIN_SNR                    = 0x27,
  PRE_RANGE_CONFIG_VALID_PHASE_LOW            = 0x56,
  PRE_RANGE_CONFIG_VALID_PHASE_HIGH           = 0x57,
  PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          = 0x64,
  FINAL_RANGE_CONFIG_MIN_SNR                  = 0x67,
  FINAL_RANGE_CONFIG_VALID_PHASE_LOW          = 0x47,
  FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         = 0x48,
  FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44,
  PRE_RANGE_CONFIG_SIGMA_THRESH_HI            = 0x61,
  PRE_RANGE_CONFIG_SIGMA_THRESH_LO            = 0x62,
  PRE_RANGE_CONFIG_VCSEL_PERIOD               = 0x50,
  PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          = 0x51,
  PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO          = 0x52,
  SYSTEM_HISTOGRAM_BIN                        = 0x81,
  HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT       = 0x33,
  HISTOGRAM_CONFIG_READOUT_CTRL               = 0x55,
  FINAL_RANGE_CONFIG_VCSEL_PERIOD             = 0x70,
  FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        = 0x71,
  FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO        = 0x72,
  CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       = 0x20,
  MSRC_CONFIG_TIMEOUT_MACROP                  = 0x46,
  SOFT_RESET_GO2_SOFT_RESET_N                 = 0xBF,
  IDENTIFICATION_MODEL_ID                     = 0xC0,
  IDENTIFICATION_REVISION_ID                  = 0xC2,
  OSC_CALIBRATE_VAL                           = 0xF8,
  GLOBAL_CONFIG_VCSEL_WIDTH                   = 0x32,
  POWER_MANAGEMENT_GO1_POWER_FORCE            = 0x80,
  VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           = 0x89,
  ALGO_PHASECAL_LIM                           = 0x30,
  ALGO_PHASECAL_CONFIG_TIMEOUT                = 0x30,
  RESULT_RANGE_MM                             = 0x1E, 
};


typedef struct {
    i2c_inst_t* i2c;
    uint8_t address;
    uint16_t io_timeout;
    uint8_t stop_variable;
    uint32_t measurement_timing_budget_us;
} vl53l0x_dev;

// Funções públicas
bool vl53l0x_init(vl53l0x_dev* dev, i2c_inst_t* i2c_port);
uint16_t vl53l0x_read_range_single_millimeters(vl53l0x_dev* dev);
void vl53l0x_start_continuous(vl53l0x_dev* dev, uint32_t period_ms);
uint16_t vl53l0x_read_range_continuous_millimeters(vl53l0x_dev* dev);

#endif
