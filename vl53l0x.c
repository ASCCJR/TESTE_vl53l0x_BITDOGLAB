/**
 * @file vl53l0x.c
 * @brief Implementação do driver para o sensor de distância ToF VL53L0X.
 *
 * Contém a lógica de baixo nível para a comunicação I2C e a complexa
 * sequência de inicialização baseada na API da STMicroelectronics.
 */

/**
 * https://github.com/ASCCJR
 */

#include "vl53l0x.h"
#include "pico/stdlib.h"
#include <string.h>

// --- Funções Helper de Baixo Nível I2C ---

static void write_reg(vl53l0x_dev* dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(dev->i2c, dev->address, buf, 2, false);
}

static void write_reg16(vl53l0x_dev* dev, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (val >> 8), (val & 0xFF)};
    i2c_write_blocking(dev->i2c, dev->address, buf, 3, false);
}

static uint8_t read_reg(vl53l0x_dev* dev, uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);
    i2c_read_blocking(dev->i2c, dev->address, &val, 1, false);
    return val;
}

static uint16_t read_reg16(vl53l0x_dev* dev, uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);
    i2c_read_blocking(dev->i2c, dev->address, buf, 2, false);
    return ((uint16_t)buf[0] << 8) | buf[1];
}


// --- Funções Públicas ---

bool vl53l0x_init(vl53l0x_dev* dev, i2c_inst_t* i2c_port) {
    dev->i2c = i2c_port;
    dev->address = VL53L0X_ADDRESS;
    dev->io_timeout = 1000; // Timeout de 1 segundo para operações.

    // A sequência abaixo é uma implementação complexa e específica do VL53L0X,
    // necessária para calibrar e configurar corretamente o sensor.
    // É baseada na API oficial da ST e em drivers de código aberto (ex: Pololu, Adafruit).

    // "Acorda" o sensor e salva a variável de parada.
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, SYSRANGE_START, 0x00);
    dev->stop_variable = read_reg(dev, 0x91);
    write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // Configurações de tuning e limites.
    write_reg(dev, MSRC_CONFIG_CONTROL, read_reg(dev, MSRC_CONFIG_CONTROL) | 0x12);
    write_reg16(dev, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)(0.25 * (1 << 7)));
    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xFF);

    // Calibração de SPAD (Single Photon Avalanche Diode).
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01); write_reg(dev, 0xFF, 0x01);
    write_reg(dev, SYSRANGE_START, 0x00); write_reg(dev, 0xFF, 0x06);
    write_reg(dev, 0x83, read_reg(dev, 0x83) | 0x04);
    write_reg(dev, 0xFF, 0x07); write_reg(dev, 0x81, 0x01);
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01); write_reg(dev, 0x94, 0x6b);
    write_reg(dev, 0x83, 0x00);
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (read_reg(dev, 0x83) == 0x00) {
        if (to_ms_since_boot(get_absolute_time()) - start > dev->io_timeout) return false;
    }
    write_reg(dev, 0x83, 0x01);
    read_reg(dev, 0x92);
    write_reg(dev, 0x81, 0x00); write_reg(dev, 0xFF, 0x06);
    write_reg(dev, 0x83, read_reg(dev, 0x83) & ~0x04);
    write_reg(dev, 0xFF, 0x01); write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00); write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // Configuração da interrupção (não usada ativamente, mas parte da sequência).
    write_reg(dev, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    write_reg(dev, GPIO_HV_MUX_ACTIVE_HIGH, read_reg(dev, GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

    // Configuração do timing budget (orçamento de tempo por medição).
    dev->measurement_timing_budget_us = 33000; // 33ms é um bom valor padrão.
    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xE8);
    write_reg16(dev, 0x04, 33000 / 1085); // Valor de aproximação para o período.

    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return true;
}

uint16_t vl53l0x_read_range_single_millimeters(vl53l0x_dev* dev) {
    // Sequência de trigger para medição única.
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(dev, 0xFF, 0x01); write_reg(dev, SYSRANGE_START, 0x00);
    write_reg(dev, 0x91, dev->stop_variable); write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00); write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);
    write_reg(dev, SYSRANGE_START, 0x01);

    // Espera o sensor ficar pronto.
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (read_reg(dev, SYSRANGE_START) & 0x01) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > dev->io_timeout) return 65535;
    }

    // Espera o dado estar disponível.
    start = to_ms_since_boot(get_absolute_time());
    while ((read_reg(dev, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > dev->io_timeout) return 65535;
    }

    uint16_t range = read_reg16(dev, RESULT_RANGE_MM);
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return range;
}

void vl53l0x_start_continuous(vl53l0x_dev* dev, uint32_t period_ms) {
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(dev, 0xFF, 0x01); write_reg(dev, SYSRANGE_START, 0x00);
    write_reg(dev, 0x91, dev->stop_variable); write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00); write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    if (period_ms != 0) {
        // Modo contínuo com intervalo programado.
        write_reg16(dev, SYSTEM_INTERMEASUREMENT_PERIOD, period_ms * 12 / 13);
        write_reg(dev, SYSRANGE_START, 0x04);
    } else {
        // Modo contínuo mais rápido possível ("back-to-back").
        write_reg(dev, SYSRANGE_START, 0x02);
    }
}

uint16_t vl53l0x_read_range_continuous_millimeters(vl53l0x_dev* dev) {
    // Espera pelo flag de "dado pronto".
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while ((read_reg(dev, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > dev->io_timeout) return 65535;
    }
    uint16_t range = read_reg16(dev, RESULT_RANGE_MM);
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return range;
}
