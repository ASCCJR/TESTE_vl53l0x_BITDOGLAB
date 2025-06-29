#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x.h"

#define I2C_PORT i2c1
const uint I2C_SDA_PIN = 2;
const uint I2C_SCL_PIN = 3;

int main() {
    stdio_init_all();
    sleep_ms(4000);

    printf("--- Iniciando Sensor VL53L0X (Modo Contínuo) ---\n");

    i2c_init(I2C_PORT, 400 * 1000); // Podemos até aumentar a velocidade do I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    vl53l0x_dev sensor_dev;
    if (!vl53l0x_init(&sensor_dev, I2C_PORT)) {
        printf("ERRO: Falha ao inicializar o sensor VL53L0X.\n");
        while (1);
    }
    printf("Sensor VL53L0X inicializado com sucesso.\n");

    // Inicia o modo de medição contínua (0ms = o mais rápido possível)
    vl53l0x_start_continuous(&sensor_dev, 0);
    printf("Sensor em modo contínuo. Coletando dados...\n");

    while (1) {
        // Apenas lê o último resultado, sem pedir uma nova medição
        uint16_t distance = vl53l0x_read_range_continuous_millimeters(&sensor_dev);
        
        if (distance == 65535) {
            printf("Timeout ou erro de leitura.\n");
        } else {
            // Um valor muito alto (>8000) geralmente indica "fora de alcance"
            if (distance > 8000) {
                 printf("Fora de alcance.\n");
            } else {
                 printf("Distancia: %d mm\n", distance);
            }
        }
        sleep_ms(100); // Apenas uma pequena pausa para não sobrecarregar o monitor
    }
    return 0;
}