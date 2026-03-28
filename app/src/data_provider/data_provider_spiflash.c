#include <stdio.h>
#include "data_provider.h"
#include "SWM341.h"

#define FLASH_BLOCK_SIZE	4096

static uint32_t flash_cached_addr = 0xFFFFFFFF;
static uint8_t flash_cache[FLASH_BLOCK_SIZE] __attribute__((aligned(8)));

static __INLINE uint32_t min(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

int data_provider_spiflash_read(uint32_t offset, void *void_buf, uint32_t in_size, uint32_t always_zero)
{
    uint8_t *buf = (uint8_t *)void_buf;
    uint32_t block_addr_start = offset / FLASH_BLOCK_SIZE * FLASH_BLOCK_SIZE;
	uint32_t block_addr_end = (offset + in_size) / FLASH_BLOCK_SIZE * FLASH_BLOCK_SIZE;
	
    uint32_t size = in_size;
	for(uint32_t block_addr = block_addr_start; block_addr <= block_addr_end; block_addr += FLASH_BLOCK_SIZE) {
		if(flash_cached_addr != block_addr) {
			flash_cached_addr = block_addr;
			
			SFC_Read(flash_cached_addr, (uint32_t *)flash_cache, FLASH_BLOCK_SIZE / 4);
		}
		
		uint32_t in_block_offset = offset - flash_cached_addr;
		uint32_t in_block_size = min(FLASH_BLOCK_SIZE - in_block_offset, size);
		
		memcpy(buf, &flash_cache[in_block_offset], in_block_size);
		buf += in_block_size;
		offset += in_block_size;
        size -= in_block_size;
		
		if(size == 0) break;
	}
    
    return in_size - size;
}
