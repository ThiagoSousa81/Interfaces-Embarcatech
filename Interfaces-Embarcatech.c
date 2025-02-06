#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define LED_PIN_G 11   // Pino do verde do LED RGB
#define LED_PIN_B 12   // Pino do azul do LED RGB
const uint BUTTON_A = 5; // Pino GPIO do botão A
const uint BUTTON_B = 6; // Pino GPIO do botão B

// Armazena o tempo do último click de botão (em microssegundos)
// O tipo volatile é para a variável ser acessada em modo assíncrono
static volatile uint32_t last_time = 0;
static volatile bool led_state_G = false, led_state_B = false;

// Função de interrupção IRQ com debouncing dos botões A e B
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 300000) // 300 ms de debouncing
    {        
        if (gpio == BUTTON_A)
        {
            led_state_G = !led_state_G;
            gpio_put(LED_PIN_G, led_state_G);

            printf("Estado do LED verde alterado!");
            // Enviar o que acontece para o display
            
            last_time = current_time; // Atualiza o tempo do último evento
        }
        else
        {
            led_state_B = !led_state_B;
            gpio_put(LED_PIN_B, led_state_B);

            printf("Estado do LED azul alterado!");
            // Enviar o que acontece para o display
            
            last_time = current_time; // Atualiza o tempo do último evento
        }
    }
}


int main()
{
    stdio_init_all(); // Inicializar a comunicação serial

    // Inicializar o pino GPIO13
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, true);

    gpio_init(LED_PIN_B);
    gpio_set_dir(LED_PIN_B, true);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_A);          // Habilita o pull-up interno
    // Configuração da interrupção com callback do botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);    

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_B);          // Habilita o pull-up interno
    // Configuração da interrupção com callback do botão B
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);





    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    while (true) {
        //uart_puts(UART_ID, " Hello, UART!\n");
        //sleep_ms(1000);
    }
}
