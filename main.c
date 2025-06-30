/**
 * @file main.c
 * @brief Aplicação de teste para o driver do sensor de distância VL53L0X.
 *
 * Este programa inicializa o sensor, o coloca em modo de medição contínua
 * e exibe a distância medida em milímetros no monitor serial.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x.h"

// --- Configuração da Porta I2C da BitDogLab ---
// Alterne os comentários para escolher a porta desejada.
#define I2C_PORT i2c1
const uint I2C_SDA_PIN = 2;
const uint I2C_SCL_PIN = 3;

// #define I2C_PORT i2c0
// const uint I2C_SDA_PIN = 0;
// const uint I2C_SCL_PIN = 1;


int main() {
    stdio_init_all();
    // Espera ativa pela conexão do monitor serial.
    while(!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("--- Iniciando Sensor VL53L0X (Modo Contínuo) ---\n");
    printf("Usando a porta I2C com SDA no pino %d e SCL no pino %d\n", I2C_SDA_PIN, I2C_SCL_PIN);

    // Inicializa o I2C. 100kHz é uma velocidade segura para depuração.
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o sensor. É crucial verificar o retorno desta função.
    vl53l0x_dev sensor_dev;
    if (!vl53l0x_init(&sensor_dev, I2C_PORT)) {
        printf("ERRO: Falha ao inicializar o sensor VL53L0X.\n");
        while (1);
    }
    printf("Sensor VL53L0X inicializado com sucesso.\n");

    // Inicia o modo de medição contínua (0ms = o mais rápido possível).
    vl53l0x_start_continuous(&sensor_dev, 0);
    printf("Sensor em modo contínuo. Coletando dados...\n");

    while (1) {
        // Apenas lê o último resultado da medição contínua.
        uint16_t distance = vl53l0x_read_range_continuous_millimeters(&sensor_dev);
        
        // Trata os possíveis valores de erro/timeout retornados pelo driver.
        if (distance == 65535) {
            printf("Timeout ou erro de leitura.\n");
        } else {
            // Um valor muito alto (>8000) geralmente indica que o alvo está fora do alcance do sensor.
            if (distance > 8000) {
                 printf("Fora de alcance.\n");
            } else {
                 printf("Distancia: %d mm\n", distance);
            }
        }
        // Pausa para não sobrecarregar o monitor serial.
        sleep_ms(100); 
    }
    return 0;
}