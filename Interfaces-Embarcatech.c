#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include <pico/stdio_usb.h>
// Bibliotecas para o display
#include "inc/ssd1306.h"
#include "inc/font.h"
//Bibliotecas para a matriz
#include "hardware/pio.h"
#include "ws2818b.pio.h"

#define LED_PIN 7        // Pino GPIO conectado aos LEDs
#define LED_COUNT 25     // Número de LEDs na matriz

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
ssd1306_t SSD; // Inicializa a estrutura do display


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


void i2c_initi()
{
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line

    
    ssd1306_init(&SSD, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&SSD); // Configura o display
    ssd1306_send_data(&SSD); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&SSD, false);
    ssd1306_send_data(&SSD);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    bool cor = false;
    cor = !cor;
    ssd1306_fill(&SSD, !cor); // Limpa o display
    ssd1306_rect(&SSD, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&SSD, "VERDE OFF", 24, 18); 
    ssd1306_draw_string(&SSD, "CHAR 0", 24, 30); // Desenha uma string
    ssd1306_draw_string(&SSD, "AZUL OFF", 24, 42); 
    ssd1306_send_data(&SSD); // Atualiza o display
}


/*========================Funções da matriz========================*/

// Estrutura para representar um pixel com componentes RGB
struct pixel_t
{
    uint8_t G, R, B; // Componentes de cor: Verde, Vermelho e Azul
};

typedef struct pixel_t pixel_t; // Alias para a estrutura pixel_t
typedef pixel_t npLED_t;        // Alias para facilitar o uso no contexto de LEDs

npLED_t leds[LED_COUNT]; // Array para armazenar o estado de cada LED
PIO np_pio;              // Variável para referenciar a instância PIO usada
uint sm;                 // Variável para armazenar o número da máquina de estado (State Machine)

// Função para inicializar o PIO para controle dos LEDs
void npInit(uint pin)
{
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carregar o programa PIO
    np_pio = pio0;                                         // Usar o primeiro bloco PIO

    sm = pio_claim_unused_sm(np_pio, false); // Tentar usar uma state machine do pio0
    if (sm < 0)                              // Se não houver disponível no pio0
    {
        np_pio = pio1;                          // Mudar para o pio1
        sm = pio_claim_unused_sm(np_pio, true); // Usar uma state machine do pio1
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializar state machine para LEDs

    // Inicializar todos os LEDs como apagados
    for (uint i = 0; i < LED_COUNT; ++i) 
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Função para atualizar os LEDs no hardware
void npUpdate()
{
    for (uint i = 0; i < LED_COUNT; ++i) // Iterar sobre todos os LEDs
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].R); // Enviar componente vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].G); // Enviar componente verde        
        pio_sm_put_blocking(np_pio, sm, leds[i].B); // Enviar componente azul
    }
}

// Função para definir a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r; // Definir componente vermelho
    leds[index].G = g; // Definir componente verde
    leds[index].B = b; // Definir componente azul
}

// Função para limpar (apagar) todos os LEDs
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i) // Iterar sobre todos os LEDs
        npSetLED(i, 0, 0, 0);            // Definir cor como preta (apagado)
}

// Função para definir qual número mostrar na tela
void setDisplayNum(char num, const uint8_t r, const uint8_t g, const uint8_t b)
{
/*  Gabarito do Display
    24, 23, 22, 21, 20
    15, 16, 17, 18, 19
    14, 13, 12, 11, 10
    05, 06, 07, 08, 09
    04, 03, 02, 01, 00
*/
    npClear();
    switch (num)
    {
    case '0': // Número 0
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);
        npSetLED(6, r, g, b);
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(16, r, g, b);
        npSetLED(18, r, g, b);        
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);
        break;
    case '1': // Número 1
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);
        npSetLED(7, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(16, r, g, b);
        npSetLED(17, r, g, b);
        npSetLED(22, r, g, b);
        break;
    case '2': // Número 2
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);
        npSetLED(6, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(18, r, g, b);
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);
        break;
    case '3': // Número 3
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(18, r, g, b);
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);
        break;
    case '4': // Número 4
        npSetLED(2, r, g, b);
        npSetLED(5, r, g, b);
        npSetLED(6, r, g, b);
        npSetLED(7, r, g, b);
        npSetLED(8, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(14, r, g, b);
        npSetLED(16, r, g, b);
        npSetLED(17, r, g, b);
        npSetLED(22, r, g, b);
        break;
    case '5': // Número 5
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);        
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(16, r, g, b); 
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);
        break;
    case '6': // Número 6
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b); 
        npSetLED(6, r, g, b);
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(16, r, g, b); 
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);
        break;
    case '7': // Número 7
        npSetLED(1, r, g, b);
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(18, r, g, b);
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);        
        break;
    case '8': // Número 8
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);
        npSetLED(6, r, g, b);
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(16, r, g, b);
        npSetLED(18, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(21, r, g, b);
        npSetLED(23, r, g, b);
        break;
    case '9': // Número 9
        npSetLED(1, r, g, b);
        npSetLED(2, r, g, b);
        npSetLED(3, r, g, b);        
        npSetLED(8, r, g, b);
        npSetLED(11, r, g, b);
        npSetLED(12, r, g, b);
        npSetLED(13, r, g, b);
        npSetLED(16, r, g, b);
        npSetLED(18, r, g, b);
        npSetLED(21, r, g, b);
        npSetLED(22, r, g, b);
        npSetLED(23, r, g, b);
        break;
    } // default desnecessário
    npUpdate(); // Atualiza o display após colocar o número
}

/*=====================================Interrupções=====================================*/

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
            if (led_state_G)
            {
                ssd1306_draw_string(&SSD, "VERDE ON ", 24, 18); 
            }
            else
            {
                ssd1306_draw_string(&SSD, "VERDE OFF", 24, 18); 
            }            
            ssd1306_send_data(&SSD); // Atualiza o display
            
            last_time = current_time; // Atualiza o tempo do último evento
        }
        else
        {
            led_state_B = !led_state_B;
            gpio_put(LED_PIN_B, led_state_B);

            printf("Estado do LED azul alterado!");
            // Enviar o que acontece para o display
            if (led_state_B)
            {
                ssd1306_draw_string(&SSD, "AZUL ON ", 24, 42); 
            }
            else
            {
                ssd1306_draw_string(&SSD, "AZUL OFF", 24, 42); 
            }
            ssd1306_send_data(&SSD); // Atualiza o display
            
            last_time = current_time; // Atualiza o tempo do último evento
        }
    }
}


int main()
{
    stdio_init_all(); // Inicializar a comunicação serial

    npInit(LED_PIN);  // Inicializar os LEDs

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

    
    i2c_initi();
    while (true) {                        
        if (stdio_usb_connected())
        { // Certifica-se de que o USB está conectado
            char c;
            if (scanf("%c", &c) == 1)
            { // Lê caractere da entrada padrão
                ssd1306_draw_char(&SSD, c, 64, 30); 
                ssd1306_send_data(&SSD); // Atualiza o display

                setDisplayNum(c, 100, 100, 100);
                
            }
        }
    }
}
