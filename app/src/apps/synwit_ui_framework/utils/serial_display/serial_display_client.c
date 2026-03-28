#include <stdio.h>
#include "serial_display_client.h"

#define CRC16_ENABLED   1

#define TAKE(a, n)  (uint8_t)(((a) >> (n)) & 0xFF)

static uint8_t* sd_frame_buf;
static uint32_t sd_frame_buf_remaining;

static uint32_t sd_frame_len;
static uint8_t sd_is_getter;

static uint16_t crc16_ccitt(const uint8_t* msg, uint32_t len)
{
    uint16_t crc = 0x0000;
    uint16_t polynomial = 0x1021;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= (msg[i] << 8);
        for (unsigned char j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            }
            else {
                crc <<= 1;
            }
        }
    }
    return crc & 0xFFFF;
}

static void _synwit_sdcmd_begin(uint8_t* buffer, uint32_t size, uint8_t msgid, uint8_t is_getter)
{
    if (buffer == NULL || size < SD_FRAME_INSTRU_START_POS) {
        sd_frame_buf = NULL;
        sd_frame_buf_remaining = 0;
        sd_frame_len = 0;
        return;
    }

    sd_frame_buf = buffer;
    sd_frame_buf_remaining = size;

    sd_frame_len = 0;
    sd_is_getter = is_getter;
    /* Header */
    if (is_getter) {
        sd_frame_buf[sd_frame_len++] = 0xFA;
        sd_frame_buf[sd_frame_len++] = 0x81;
    }
    else {
        sd_frame_buf[sd_frame_len++] = 0xFA;
        sd_frame_buf[sd_frame_len++] = 0x80;
    }
    
    /* Data len */
    sd_frame_buf[sd_frame_len++] = 0;
    sd_frame_buf[sd_frame_len++] = 0;

    /* Msg id */
    sd_frame_buf[sd_frame_len++] = msgid;

    /* Num of instruction(s) */
    sd_frame_buf[sd_frame_len++] = 0;

    sd_frame_buf_remaining -= sd_frame_len;
}

void synwit_sdcmd_setter_begin(uint8_t* buffer, uint32_t size, uint8_t msgid)
{
    _synwit_sdcmd_begin(buffer, size, msgid, 0);
}

void synwit_sdcmd_getter_begin(uint8_t* buffer, uint32_t size, uint8_t msgid)
{
    _synwit_sdcmd_begin(buffer, size, msgid, 1);
}

void _synwit_sdcmd_add(sdcode_t op_code, uint32_t object_id, uint32_t value)
{
    uint8_t data_type;
    uint8_t* str;
    uint16_t str_len;
    uint32_t str_len_pos;

    if (sd_frame_buf == NULL || sd_frame_buf_remaining < 10) {
        return;
    }

    sd_frame_buf_remaining -= 10;

    /* Object id */
    sd_frame_buf[sd_frame_len++] = TAKE(object_id, 0);
    sd_frame_buf[sd_frame_len++] = TAKE(object_id, 8);

    /* Property code */
    sd_frame_buf[sd_frame_len++] = TAKE(op_code, 0);
    sd_frame_buf[sd_frame_len++] = TAKE(op_code, 8);

    if (sd_is_getter == 0) {
        data_type = TAKE(op_code, 12);
        if (data_type == PTYPE_STRING) {
            str_len_pos = sd_frame_len;

            /* Skip string len field*/
            sd_frame_len += 2;

            /* Write string */
            str = (uint8_t*)value;
            while (*str && sd_frame_buf_remaining--) {
                sd_frame_buf[sd_frame_len++] = *str++;
            }

            /* String null terminator */
            if (sd_frame_buf_remaining) {
                sd_frame_buf[sd_frame_len++] = 0;
                sd_frame_buf_remaining--;
            }

            /* Re-write string len */
            str_len = (uint16_t)((uint32_t)str - value);
            sd_frame_buf[str_len_pos] = TAKE(str_len, 0);
            sd_frame_buf[str_len_pos + 1] = TAKE(str_len, 8);
        }
        else {
            sd_frame_buf[sd_frame_len++] = TAKE(value, 0);
            sd_frame_buf[sd_frame_len++] = TAKE(value, 8);
            sd_frame_buf[sd_frame_len++] = TAKE(value, 16);
            sd_frame_buf[sd_frame_len++] = TAKE(value, 24);
        }
    }

    /* Increase num of instructions */
    sd_frame_buf[SD_FRAME_CMD_NUM_POS]++;
}

uint8_t* synwit_sdcmd_end(uint32_t* len)
{
    /* Update data len field */
    uint16_t data_len = (uint16_t)(sd_frame_len - 4);
    sd_frame_buf[2] = TAKE(data_len, 0);
    sd_frame_buf[3] = TAKE(data_len, 8);

    uint32_t extra_len = 0;
#if (CRC16_ENABLED == 1)
    uint16_t crc16 = crc16_ccitt(sd_frame_buf, sd_frame_len);
    sd_frame_buf[sd_frame_len] = TAKE(crc16, 0);
    sd_frame_buf[sd_frame_len + 1] = TAKE(crc16, 8);
    extra_len = 2;
#endif

    if (len) {
        *len = sd_frame_len + extra_len;
    }
    return sd_frame_buf;
}

