#define AUDIO_DECODER_C
#include "synwit_ui_framework/hal/audio_decoder.h"
#include "board.h"
#include "SWM341.h"
#include "lvgl/lvgl.h"

#define I2S_MUTE_PORT       GPIOD
#define I2S_MUTE_PIN        PIN11

static volatile uint8_t i2s_dma_running = 0;
static uint8_t i2s_inited = 0;

static inline void wait_i2s_dma()
{
    while(i2s_dma_running) __NOP();
}

static void hw_i2s_init(uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample)
{
    if(i2s_inited == 0) {
        wait_i2s_dma();
        
        i2s_init(channels, bits_per_sample, sample_rate);
        i2s_inited = 1;
    }
}

static void* audio_decoder_open(uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample)
{
    hw_i2s_init(channels, sample_rate, bits_per_sample);
    //printf("channels[%d], bps[%d], sample_rate[%d]\r\n", channels, bits_per_sample, sample_rate);
    return (void *)1;
}

static void audio_decoder_close(void *decoder_obj)
{
    if(decoder_obj == NULL) {
        return;
    }

    wait_i2s_dma();
    i2s_deinit();

    i2s_inited = 0;
}

void DMA_Handler(void)
{
    if(0 == i2s_dma_done()) {
        uint32_t size;
        uint32_t bits_per_sample;
        uint8_t* next = _audio_manager_next_frame(&size, NULL, NULL, &bits_per_sample);
        
        if(next) {
            uint32_t count = (bits_per_sample == 8) ? size : 
            ( (bits_per_sample == 16) ? size / 2 : 
            ( (bits_per_sample == 24) ? size / 3 : 
            ( (bits_per_sample == 32) ? size / 4 : 0)
            )
            );
            
            i2s_dma_init(next, count);
        } else {
            i2s_dma_running = 0;
        }
    }
}

static void audio_decoder_notify(void* decoder_obj, audio_notification_t noti)
{
    if(i2s_dma_running == 0) {
        i2s_dma_running = 1;

        uint32_t size;
        uint32_t bits_per_sample;
        uint8_t* next = _audio_manager_next_frame(&size, NULL, NULL, &bits_per_sample);
        
        if(next) {
            uint32_t count = (bits_per_sample == 8) ? size : 
            ( (bits_per_sample == 16) ? size / 2 : 
            ( (bits_per_sample == 24) ? size / 3 : 
            ( (bits_per_sample == 32) ? size / 4 : 0)
            )
            );
            i2s_dma_init(next, count);
        } else {
            i2s_dma_running = 0;
        }
    }
}

static void audio_decoder_wait_flush(void* decoder_obj)
{
    wait_i2s_dma();
}

static int audio_decoder_is_on_flushing(void* decoder_obj)
{
    return (i2s_dma_running ? 1 : 0);
}

static audio_decoder_t avi_audio_decoder = {
    .open = audio_decoder_open,
    .close = audio_decoder_close,
    .notify = audio_decoder_notify,
    .wait_flush = audio_decoder_wait_flush,
    .is_on_flushing = audio_decoder_is_on_flushing,
    
    .pcm_zero_point_offset = 0,
    .force_mode = AUDIO_DECODER_FORCE_STEREO,
};

audio_decoder_t *get_default_dac_decoder()
{
    return &avi_audio_decoder;
}


