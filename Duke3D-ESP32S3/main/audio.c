/*
 *  I2S audio for the ESP32-S3 port: MAX98357A amplifier (T-Deck) or ES8311
 *  codec (Wio Tracker L2), mono speakers.
 *
 *  The MG24 port played audioBuffer with a timer-paced DMA and let the mixer
 *  read the DMA source pointer as its play cursor. Here a task on core 0
 *  drains the same ring into I2S in small blocks and advances the cursor; the
 *  blocking I2S write paces it at the sample rate.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "audio.h"
#include "board.h"

#define BLOCK_FRAMES        64
#define AUDIO_DEBUG         0

AUX_SECTION uint8_t audioBuffer[AUDIO_BUFFER_LENGTH * 2];
static i2s_chan_handle_t tx;
static volatile uint32_t playIndex;

uint32_t audioGetPlayIndex(void)
{
    return playIndex;
}

int SNDDMA_GetDMAPos(void)
{
    return playIndex;
}

static void audioTask(void *arg)
{
    (void) arg;
    int16_t out[BLOCK_FRAMES * 2];
    uint32_t idx = 0;
    while (1)
    {
        for (int i = 0; i < BLOCK_FRAMES; i++)
        {
            uint32_t f = (idx + i) & (AUDIO_BUFFER_LENGTH - 1);
            int32_t l = (int32_t) audioBuffer[2 * f] - 128;
            int32_t r = (int32_t) audioBuffer[2 * f + 1] - 128;
            int16_t m = (int16_t) (((l + r) << AUDIO_OUTPUT_SHIFT) >> 1);
            out[2 * i] = m;
            out[2 * i + 1] = m;
        }
#if AUDIO_DEBUG
        static int32_t peak;
        static uint32_t blocks;
        for (int i = 0; i < BLOCK_FRAMES; i++)
        {
            int32_t a = out[2 * i] < 0 ? -out[2 * i] : out[2 * i];
            peak = a > peak ? a : peak;
        }
        if (++blocks % 512 == 0)
        {
            ESP_LOGI("audio", "peak %ld, play idx %lu", (long) peak, (unsigned long) idx);
            peak = 0;
        }
#endif
        idx = (idx + BLOCK_FRAMES) & (AUDIO_BUFFER_LENGTH - 1);
        // The mixer may now overwrite what was just copied.
        playIndex = idx;
        size_t written;
        i2s_channel_write(tx, out, sizeof(out), &written, portMAX_DELAY);
    }
}

void initAudio(void)
{
    static bool done;
    if (done)
    {
        return;
    }
    done = true;
    memset(audioBuffer, 128, sizeof(audioBuffer));
    i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan.dma_desc_num = 4;
    chan.dma_frame_num = BLOCK_FRAMES;
    chan.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan, &tx, NULL));
    i2s_std_config_t std =
    {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg =
        {
            .mclk = PIN_I2S_MCLK,
            .bclk = PIN_I2S_BCK,
            .ws = PIN_I2S_WS,
            .dout = PIN_I2S_DOUT,
            .din = I2S_GPIO_UNUSED,
        },
    };
    std.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;     // what the ES8311 setup expects
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &std));
    ESP_ERROR_CHECK(i2s_channel_enable(tx));
    boardAudioCodecInit();      // needs MCLK running
    xTaskCreatePinnedToCore(audioTask, "audio", 3072, NULL, 20, NULL, 0);
    ESP_LOGI("audio", "I2S started at %d Hz", AUDIO_SAMPLE_RATE);
}
