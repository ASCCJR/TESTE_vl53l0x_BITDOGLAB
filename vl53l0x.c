// Versão melhorada com nomes de registradores (enum) para maior legibilidade.
#include "vl53l0x.h"
#include "pico/stdlib.h"
#include <string.h>

// --- Funções de baixo nível para I2C usando Pico SDK ---
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

// --- Inicialização completa do sensor ---
bool vl53l0x_init(vl53l0x_dev* dev, i2c_inst_t* i2c_port) {
    dev->i2c = i2c_port;
    dev->address = VL53L0X_ADDRESS;
    dev->io_timeout = 1000;

    // "Acorda" o sensor e salva a variável de parada
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, SYSRANGE_START, 0x00);
    dev->stop_variable = read_reg(dev, 0x91);
    write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // Desativa limit checks
    write_reg(dev, MSRC_CONFIG_CONTROL, read_reg(dev, MSRC_CONFIG_CONTROL) | 0x12);

    // Limite de rate de sinal (0.25 MCPS)
    write_reg16(dev, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)(0.25 * (1 << 7)));

    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xFF);

    // Sequência de tuning robusta
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
    read_reg(dev, 0x92); // Leitura de SPAD info

    write_reg(dev, 0x81, 0x00); write_reg(dev, 0xFF, 0x06);
    write_reg(dev, 0x83, read_reg(dev, 0x83) & ~0x04);
    write_reg(dev, 0xFF, 0x01); write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00); write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // Configuração de interrupção
    write_reg(dev, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    write_reg(dev, GPIO_HV_MUX_ACTIVE_HIGH, read_reg(dev, GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

    dev->measurement_timing_budget_us = 33000;
    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xE8);
    write_reg16(dev, 0x04, 33000 / 1085);

    // Interrompe continuous se estava ativo
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

    return true;
}

// --- Medição single robusta ---
uint16_t vl53l0x_read_range_single_millimeters(vl53l0x_dev* dev) {
    // Sequência de trigger single robusta
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, SYSRANGE_START, 0x00);
    write_reg(dev, 0x91, dev->stop_variable);
    write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);
    write_reg(dev, SYSRANGE_START, 0x01);

    // Espera sensor estar pronto (não ocupado)
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (read_reg(dev, SYSRANGE_START) & 0x01) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > dev->io_timeout)
            return 65535; // Timeout
    }

    // Espera dado pronto (flag de interrupção)
    start = to_ms_since_boot(get_absolute_time());
    while ((read_reg(dev, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > dev->io_timeout)
            return 65535; // Timeout
    }

    // Lê a distância do registrador de resultado
    uint16_t range = read_reg16(dev, RESULT_RANGE_MM);

    // Limpa a flag de interrupção
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

    return range;
}

// --- Modo contínuo ---
void vl53l0x_start_continuous(vl53l0x_dev* dev, uint32_t period_ms) {
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, SYSRANGE_START, 0x00);
    write_reg(dev, 0x91, dev->stop_variable);
    write_reg(dev, SYSRANGE_START, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    if (period_ms != 0) {
        // Modo com intervalo programado
        write_reg16(dev, SYSTEM_INTERMEASUREMENT_PERIOD, period_ms * 12 / 13); // Aproximação
        write_reg(dev, SYSRANGE_START, 0x04);
    } else {
        // Modo contínuo mais rápido possível
        write_reg(dev, SYSRANGE_START, 0x02);
    }
}

uint16_t vl53l0x_read_range_continuous_millimeters(vl53l0x_dev* dev) {
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while ((read_reg(dev, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > dev->io_timeout)
            return 65535;
    }
    uint16_t range = read_reg16(dev, RESULT_RANGE_MM);
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return range;
}