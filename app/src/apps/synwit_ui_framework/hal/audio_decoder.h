#ifndef SYNWIT_AUDIO_DECODER_DECLARE_H
#define SYNWIT_AUDIO_DECODER_DECLARE_H
#include <stdint.h>
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	AUDIO_DECODER_NOTIFY_NEW_DATA_READY = 0,
};
typedef uint8_t audio_notification_t;

enum {
	AUDIO_DECODER_FORCE_NONE = 0,
	AUDIO_DECODER_FORCE_STEREO,
	AUDIO_DECODER_FORCE_MONO
};
typedef uint8_t audio_force_mode_t;

typedef struct {
	void* (*open)(uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample);
	void (*close)(void* decoder_obj);

	void (*notify)(void* decoder_obj, audio_notification_t noti);
	void (*wait_flush)(void* decoder_obj);
	int (*is_on_flushing)(void* decoder_obj);

	int pcm_zero_point_offset;
	audio_force_mode_t force_mode;
} audio_decoder_t;

uint8_t* _audio_manager_next_frame(uint32_t* size, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SYNWIT_AUDIO_DECODER_DECLARE_H*/