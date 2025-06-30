# 📏 Teste Modular do Sensor de Distância ToF VL53L0X

![Linguagem](https://img.shields.io/badge/Linguagem-C-blue.svg)
![Plataforma](https://img.shields.io/badge/Plataforma-Raspberry%20Pi%20Pico-purple.svg)
![Sensor](https://img.shields.io/badge/Sensor-VL53L0X-red.svg)

Este repositório contém um código de teste para o sensor de distância I2C VL53L0X (Time-of-Flight), desenvolvido em C para o Raspberry Pi Pico (BITDOGLAB)

O projeto se destaca pela implementação de um driver robusto para um sensor de alta complexidade. A inicialização do VL53L0X requer uma sequência específica e longa de configurações, que foi portada de implementações de referência e encapsulada em um driver limpo e reutilizável (`vl53l0x.c` e `.h`).

## ✨ Funcionalidades

* **Medição de Distância Precisa:** Utiliza a tecnologia Time-of-Flight (ToF) para medir distâncias em milímetros (mm).
* **Driver Modular Profissional:** A lógica do sensor é encapsulada em um driver que utiliza uma `struct` para manter o estado do dispositivo, permitindo gerenciar múltiplos sensores.
* **Implementação Robusta:** O driver executa a complexa sequência de inicialização e calibração de fábrica, essencial para o funcionamento do sensor.
* **Modos de Leitura:** A API do driver oferece suporte tanto para medições únicas ("single-shot") quanto para medições contínuas de alta velocidade.
* **Código Legível:** Utiliza uma `enum` para todos os endereços de registradores, eliminando "números mágicos" e tornando o código do driver muito mais claro.
* **Configuração Flexível de Porta:** Permite alternar facilmente o uso entre os barramentos I2C 0 e I2C 1.

## 🛠️ Hardware e Software Necessários

### Hardware
* Placa com Raspberry Pi Pico (neste projeto, foi usada a **BitDogLab**)
* Sensor de Distância I2C **VL53L0X**

### Software
* [Raspberry Pi Pico C/C++ SDK](https://github.com/raspberrypi/pico-sdk)
* Ambiente de compilação C/C++ (GCC para ARM, CMake)
* Um programa para monitor serial (ex: o integrado no VS Code, PuTTY, etc.)

## Configurar a Porta I2C
* Por padrão, o projeto está configurado para usar o conector I2C 1 (Esquerda) da BitDogLab.
* Para trocar para o conector I2C 0 (Direita), edite o arquivo main.c
* Comente o bloco de configuração do I2C 1.
* Descomente o bloco de configuração do I2C 0.

## Exemplo para usar a porta I2C 0:

```bash
// --- Configuração da Porta I2C 1 da BitDogLab (Comentado) ---
// #define I2C_PORT i2c1
// const uint I2C_SDA_PIN = 2;
// const uint I2C_SCL_PIN = 3;

// --- Configuração da Porta I2C 0 da BitDogLab (Ativo) ---
#define I2C_PORT i2c0
const uint I2C_SDA_PIN = 0;
const uint I2C_SCL_PIN = 1;
```
## Visualizar a Saída
* Conecte o sensor TCS34725 na porta I2C configurada.
* Aponte o sensor para um objeto a uma distância de até ~1.2 metros.
* Abra um monitor serial conectado à porta COM do seu Pico. Você verá a seguinte saída:
```bash
--- Iniciando Sensor VL53L0X (Modo Contínuo) ---
Usando a porta I2C com SDA no pino 3 e SCL no pino 2
Sensor VL53L0X inicializado com sucesso.
Sensor em modo contínuo. Coletando dados...
Distancia: 616 mm
Distancia: 610 mm
Distancia: 121 mm
Distancia: 90 mm
Fora de alcance.
```
## 📂 Estrutura dos Arquivos

* main.c: A aplicação principal que inicializa e utiliza o driver para exibir as medições.
* vl53l0x.c: A implementação do driver do sensor, contendo a complexa lógica de inicialização e leitura.
* vl53l0x.h: O arquivo de cabeçalho (a interface ou API) para o driver do VL53L0X.
* CMakeLists.txt: O arquivo de build do projeto.
  
✍️ Autor

[ASSCJR]
