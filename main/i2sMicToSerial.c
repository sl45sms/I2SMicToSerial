#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "driver/i2s_std.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <stdint.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_check.h"

#define MIC_STD_BCLK_IO2    GPIO_NUM_11    // I2S bit clock io number
#define MIC_STD_WS_IO2      GPIO_NUM_2     // I2S word select io number
#define MIC_STD_DIN_IO2     GPIO_NUM_10    // I2S data in io number

#define RECORDING_BUFF_SIZE               8192

static i2s_chan_handle_t                rx_chan;        // I2S rx channel handler


static void i2s_microphone_read_task(void *args)
{
    uint8_t *rec_buf = (uint8_t *)calloc(1, RECORDING_BUFF_SIZE);
    assert(rec_buf); // Check if rec_buf allocation success

    size_t rec_bytes = 0;

    /* Enable the RX channel */
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    while (1) {
        /* Read i2s data */
        if (i2s_channel_read(rx_chan, rec_buf, RECORDING_BUFF_SIZE, &rec_bytes, 1000) == ESP_OK) {
           uart_write_bytes(UART_NUM_0, rec_buf, rec_bytes); // Write to UART
        } else {
            printf("i2s read failed\n");
        }
        taskYIELD(); //to avoid "Task watchdog got triggered"
    }
    //never reach here
    free(rec_buf);
    vTaskDelete(NULL);
}

static void i2s_init_std_rx_simplex(void)
{

    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = MIC_STD_BCLK_IO2,
            .ws   = MIC_STD_WS_IO2,
            .dout = I2S_GPIO_UNUSED,
            .din  = MIC_STD_DIN_IO2,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    /* Set data bit-width to 24 means your sample is 24 bits, I2S will help to shift the data by hardware */
    rx_std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_24BIT;
    /* Set slot bit-width to 32 means there still will be 32 bit in one slot */
    rx_std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    /* Set the mclk_multiple to a multiple of 3, so that it can calculate an integer division to a 24-bit data */
    rx_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    /* Set the slot number to 1, so that the data will be sent in one slot */
    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; // The L/R pin of MIC is pulled down, which selects left slot
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));
    
}

void init_uart0 (void)
{
    const uart_config_t uart_config = {
        .baud_rate = 921600, // 115200
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
    };              
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    // Set UART pins using UART0 default pins i.e. no changes, so keep it here commented 
    //ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // Install UART driver (we don't need an event queue here)
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, RECORDING_BUFF_SIZE * 2, 0, 0, NULL, 0));
}

void app_main(void)
{
    printf("Serial Mic (24bit PCM over UART0 port!)\n");
    init_uart0();
    i2s_init_std_rx_simplex();
    xTaskCreate(i2s_microphone_read_task, "i2s_microphone_read_task", 4096, NULL, 5, NULL);
}
