#define AUDIO_DECODER_C
#include "synwit_ui_framework/hal/audio_decoder.h"
#include "board.h"
#include "SWM341.h"
#include "lvgl/lvgl.h"

// Avoid conflict with SPI DMA display
#if !defined(SWM34SCE_A1)

#define DAC_MUTE_PORT       GPIOD
#define DAC_MUTE_PIN        PIN11

static int dac_dma_running = 0;
static int dac_inited = 0;

static inline void wait_dac_dma()
{
    while(dac_dma_running) {
        __NOP();
    }
}

static inline void dac_mute()
{
    GPIO_SetBit(DAC_MUTE_PORT, DAC_MUTE_PIN);
    for (uint32_t i = 0; i < CyclesPerUs * 1000 * 10; i++)
        __NOP();
}

static inline void dac_unmute()
{
    GPIO_ClrBit(DAC_MUTE_PORT, DAC_MUTE_PIN);
}

static void hw_dac_init(uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample)
{
    if(dac_inited == 0) {
        wait_dac_dma();
        
        GPIO_Init(DAC_MUTE_PORT, DAC_MUTE_PIN, 1, 1, 0, 0);
        PORT_Init(PORTD, PIN2, PORTD_PIN2_DAC_OUT, 0);
        
        dac_mute();

        dac_init(DAC_FORMAT_MSB12B);
        dac_dma_init((uint32_t)NULL, 
                        0, 
                        (bits_per_sample == 8) ? DMA_UNIT_BYTE : DMA_UNIT_HALFWORD,
                        sample_rate);
        dac_unmute();
        dac_inited = 1;
    }
}

static void* audio_decoder_open(uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample)
{
    hw_dac_init(channels, sample_rate, bits_per_sample);

    return (void *)1;
}

static void audio_decoder_close(void *decoder_obj)
{
    if(decoder_obj == NULL) {
        return;
    }

    dac_mute();
    wait_dac_dma();
    dac_deinit();
    TIMR_Stop(TIMR0);

    dac_inited = 0;
}

void DMA_Handler(void)
{
    if(DMA_CH_INTStat(DMA_CH0, DMA_IT_DONE)) {
        DMA_CH_INTClr(DMA_CH0, DMA_IT_DONE);
        
        uint32_t size;
        uint32_t bits_per_sample;
        uint8_t* next = _audio_manager_next_frame(&size, NULL, NULL, &bits_per_sample);
        
        if(next) {
            uint32_t count = (bits_per_sample == 8) ? size : size / 2;
            DMA->CH[DMA_CH0].CR &= ~DMA_CR_LEN_Msk;
            DMA->CH[DMA_CH0].CR |= ((count ? count - 1 : 0) << DMA_CR_LEN_Pos);
            DMA->CH[DMA_CH0].SRC = (uint32_t)next;
        } else {
            TIMR_Stop(TIMR0);
            dac_dma_running = 0;
        }
    }
}

static void audio_decoder_notify(void* decoder_obj, audio_notification_t noti)
{
    if(dac_dma_running == 0) {
        dac_dma_running = 1;
        TIMR_Start(TIMR0);
    }
}

static void audio_decoder_wait_flush(void* decoder_obj)
{
    wait_dac_dma();
}

static int audio_decoder_is_on_flushing(void* decoder_obj)
{
    return (dac_dma_running ? 1 : 0);
}

static audio_decoder_t avi_audio_decoder = {
    .open = audio_decoder_open,
    .close = audio_decoder_close,
    .notify = audio_decoder_notify,
    .wait_flush = audio_decoder_wait_flush,
    .is_on_flushing = audio_decoder_is_on_flushing,
    
    .pcm_zero_point_offset = 32768,
    .force_mode = AUDIO_DECODER_FORCE_MONO,
};

audio_decoder_t *get_default_dac_decoder()
{
    return &avi_audio_decoder;
}
#else
audio_decoder_t *get_default_dac_decoder()
{
    return NULL;
}
#endif

