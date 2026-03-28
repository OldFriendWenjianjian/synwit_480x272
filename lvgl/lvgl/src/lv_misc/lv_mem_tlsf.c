/**
 * @file lv_mem.c
 * General and portable implementation of malloc and free.
 * The dynamic memory monitoring is also supported.
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_mem.h"
#include "lv_math.h"
#include "lv_gc.h"
#include "lv_debug.h"
#include <string.h>

#include "lv_tlsf.h"

#if LV_MEM_CUSTOM != 0
    #include LV_MEM_CUSTOM_INCLUDE
#endif

/*********************
 *      DEFINES
 *********************/
#ifndef FastMem_Alloc_Max_Threshold
#define FastMem_Alloc_Max_Threshold (421) /* eg: obj, style, draw_desc... sizeof(lv_disp_t) == 420, sizeof(lv_obj_t) == 96, */
#endif

#define monitor_core_is_fast_mem

/*memset the allocated memories to 0xaa and freed memories to 0xbb (just for testing purposes)*/
#ifndef LV_MEM_ADD_JUNK
    #define LV_MEM_ADD_JUNK 0
#endif

#ifdef LV_ARCH_64
    #define MEM_UNIT         uint64_t
    #define ALIGN_MASK       0x7
#else
    #define MEM_UNIT         uint32_t
    #define ALIGN_MASK       0x3
#endif

#define ZERO_MEM_SENTINEL  0xa1b2c3d4

#ifndef WIN32
    #define DECELERATE(n) \
        { \
            volatile uint8_t a = 0; \
            for(uint8_t i8462 = 0; i8462 < (n); i8462++) { \
                a = 0; \
            } \
        }
#else
    #define DECELERATE(n)
#endif

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_tlsf_t tlsf;
    size_t cur_used;
    size_t max_used;
//    lv_ll_t  pool_ll;
} lv_tlsf_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if LV_MEM_CUSTOM == 0
    static void lv_mem_walker(void * ptr, size_t size, int used, void * user);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
#if LV_MEM_CUSTOM == 0
    lv_tlsf_state_t state;
    lv_tlsf_state_t fast_state;
#endif

static uint32_t zero_mem = ZERO_MEM_SENTINEL; /*Give the address of this variable if 0 byte should be allocated*/

static uint8_t mem_deceleration = 0;

/**********************
 *      MACROS
 **********************/
#if LV_LOG_TRACE_MEM
    #define MEM_TRACE(...) LV_LOG_TRACE(__VA_ARGS__)
#else
    #define MEM_TRACE(...)
#endif

#define COPY32 *d32 = *s32; d32++; s32++;
#define COPY8 *d8 = *s8; d8++; s8++;
#define SET32(x) *d32 = x; d32++;
#define SET8(x) *d8 = x; d8++;
#define REPEAT8(expr) expr expr expr expr expr expr expr expr

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the dyn_mem module (work memory and other variables)
 */
void _lv_mem_init(void)
{
#if LV_MEM_CUSTOM == 0

#if LV_MEM_ADR == 0
    /*Allocate a large array to store the dynamically allocated data*/
    static LV_MEM_ATTR MEM_UNIT work_mem_int[LV_MEM_SIZE / sizeof(MEM_UNIT)];
    state.tlsf = lv_tlsf_create_with_pool((void *)work_mem_int, LV_MEM_SIZE);
    
    static LV_FASTMEM_ATTR MEM_UNIT work_fastmem_int[LV_FASTMEM_SIZE / sizeof(MEM_UNIT)];
    fast_state.tlsf = lv_tlsf_create_with_pool((void *)work_fastmem_int, LV_FASTMEM_SIZE);
#else
    state.tlsf = lv_tlsf_create_with_pool((void *)LV_MEM_ADR, LV_MEM_SIZE);
    fast_state.tlsf = lv_tlsf_create_with_pool((void *)LV_FASTMEM_ADR, LV_FASTMEM_SIZE);
#endif
#endif
        
#if LV_MEM_ADD_JUNK
    LV_LOG_WARN("LV_MEM_ADD_JUNK is enabled which makes LVGL much slower");
#endif
}

/**
 * Clean up the memory buffer which frees all the allocated memories.
 * @note It work only if `LV_MEM_CUSTOM == 0`
 */
void _lv_mem_deinit(void)
{
#if LV_MEM_CUSTOM == 0
    lv_tlsf_destroy(state.tlsf);
    lv_tlsf_destroy(fast_state.tlsf);
    _lv_mem_init();
#endif
}

void* lv_mem_alloc_align(size_t size, size_t addr_alignment)
{
    (void)addr_alignment;
    MEM_TRACE("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        MEM_TRACE("using zero_mem");
        return &zero_mem;
    }
    
    lv_tlsf_state_t *state_p = (size < FastMem_Alloc_Max_Threshold) ? &fast_state : &state;
    void * alloc;
    
lv_mem_alloc_align_begin:
#if LV_MEM_CUSTOM == 0
    alloc = lv_tlsf_malloc(state_p->tlsf, size);
#else
    alloc = LV_MEM_CUSTOM_ALLOC(size);
#endif

    if(alloc == NULL) {
        LV_LOG_INFO("couldn't allocate memory (%lu bytes)", (unsigned long)size);
#if LV_LOG_LEVEL <= LV_LOG_LEVEL_INFO
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        LV_LOG_INFO("used: %6d (%3d %%), frag: %3d %%, biggest free: %6d",
                    (int)(mon.total_size - mon.free_size), mon.used_pct, mon.frag_pct,
                    (int)mon.free_biggest_size);
#endif
    }
#if LV_MEM_ADD_JUNK
    else {
        lv_memset(alloc, 0xaa, size);
    }
#endif

    if(alloc) {
#if LV_MEM_CUSTOM == 0
        state_p->cur_used += size;
        state_p->max_used = LV_MATH_MAX(state_p->cur_used, state_p->max_used);
#endif
        MEM_TRACE("allocated at %p", alloc);
    }
    else if (&fast_state == state_p) {
        state_p = &state;
        goto lv_mem_alloc_align_begin;/* fastmem_failback */
    }
    return alloc;
}

/**
 * Allocate a memory dynamically
 * @param size size of the memory to allocate in bytes
 * @return pointer to the allocated memory
 */
void * lv_mem_alloc(size_t size)
{
    return lv_mem_alloc_align(size, 0);
}


/**
 * Free an allocated data
 * @param data pointer to an allocated memory
 */
void lv_mem_free(const void * data)
{
    MEM_TRACE("freeing %p", data);
    if(data == &zero_mem) return;
    if(data == NULL) return;
    
    lv_tlsf_state_t *state_p = (data < lv_tlsf_get_pool(state.tlsf)) ? &fast_state : &state;

#if LV_MEM_CUSTOM == 0
#  if LV_MEM_ADD_JUNK
    lv_memset(data, 0xbb, lv_tlsf_block_size(data));
#  endif
    size_t size = lv_tlsf_free(state_p->tlsf, data);
    if(state_p->cur_used > size) state_p->cur_used -= size;
    else state_p->cur_used = 0;
#else
    LV_MEM_CUSTOM_FREE(data);
#endif
}

/**
 * Reallocate a memory with a new size. The old content will be kept.
 * @param data pointer to an allocated memory.
 * Its content will be copied to the new memory block and freed
 * @param new_size the desired new size in byte
 * @return pointer to the new memory
 */

#if LV_ENABLE_GC == 0

void * lv_mem_realloc(void * data_p, size_t new_size)
{
    MEM_TRACE("reallocating %p with %lu size", data_p, (unsigned long)new_size);
    if(new_size == 0) {
        MEM_TRACE("using zero_mem");
        lv_mem_free(data_p);
        return &zero_mem;
    }

    if(data_p == &zero_mem) return lv_mem_alloc(new_size);
    
    void * p_new;
    
#if LV_MEM_CUSTOM == 0
    lv_tlsf_state_t *old_state_p = (data_p < lv_tlsf_get_pool(state.tlsf)) ? &fast_state : &state;
    
    lv_tlsf_state_t *new_state_p = (new_size < FastMem_Alloc_Max_Threshold) ? &fast_state : &state;
    
    size_t old_size = lv_tlsf_block_size(data_p);
    
lv_mem_realloc_begin:

    if (old_state_p == new_state_p) {
        p_new = lv_tlsf_realloc(new_state_p->tlsf, data_p, new_size);
    }
    else {
        p_new = lv_tlsf_malloc(new_state_p->tlsf, new_size);
    }
    
    if(p_new) {
        if (old_state_p == new_state_p) {
            new_state_p->cur_used -= old_size;
        }
        else {
            if (LV_MATH_MIN(old_size, new_size)) {
                _lv_memcpy(p_new, data_p, LV_MATH_MIN(old_size, new_size));
            }
            lv_tlsf_free(old_state_p->tlsf, data_p);
            
            old_state_p->cur_used -= old_size;
            old_state_p->max_used = LV_MATH_MAX(old_state_p->cur_used, old_state_p->max_used);
        }

        new_state_p->cur_used += lv_tlsf_block_size(p_new);
        new_state_p->max_used = LV_MATH_MAX(new_state_p->cur_used, new_state_p->max_used);
    }
    else if (&fast_state == new_state_p) {
        new_state_p = &state;
        goto lv_mem_realloc_begin;/* fastmem_failback */
    }
#else
    p_new = LV_MEM_CUSTOM_REALLOC(data_p, new_size);
#endif
    
    if(p_new == NULL) {
        LV_LOG_ERROR("couldn't allocate memory");
        return NULL;
    }
    
    MEM_TRACE("allocated at %p", p_new);
    return p_new;
}

#else /* LV_ENABLE_GC */

void * lv_mem_realloc(void * data_p, size_t new_size)
{
    void * new_p = LV_MEM_CUSTOM_REALLOC(data_p, new_size);
    if(new_p == NULL) LV_LOG_WARN("Couldn't allocate memory");
    return new_p;
}

#endif /* lv_enable_gc */

/**
 * Join the adjacent free memory blocks
 */
void lv_mem_defrag(void)
{
}

lv_res_t lv_mem_test(void)
{
    if(zero_mem != ZERO_MEM_SENTINEL) {
        LV_LOG_WARN("zero_mem is written");
        return LV_RES_INV;
    }

#if LV_MEM_CUSTOM == 0
    lv_tlsf_state_t *state_p = &state;
    
lv_mem_test_begin:

    if(lv_tlsf_check(state_p->tlsf)) {
        LV_LOG_WARN("failed");
        return LV_RES_INV;
    }

    if(lv_tlsf_check_pool(lv_tlsf_get_pool(state_p->tlsf))) {
        LV_LOG_WARN("pool failed");
        return LV_RES_INV;
    }
    
    if (state_p != &fast_state) {
        state_p = &fast_state;
        goto lv_mem_test_begin;
    }
#endif
    MEM_TRACE("passed");
    return LV_RES_OK;
}

/**
 * Give information about the work memory of dynamic allocation
 * @param mon_p pointer to a dm_mon_p variable,
 *              the result of the analysis will be stored here
 */
void lv_mem_monitor(lv_mem_monitor_t * mon_p)
{
    /*Init the data*/
    _lv_memset(mon_p, 0, sizeof(lv_mem_monitor_t));
#if LV_MEM_CUSTOM == 0
    MEM_TRACE("begin");
    
#ifdef monitor_core_is_fast_mem
    lv_tlsf_state_t *state_p = &fast_state;
#else
    lv_tlsf_state_t *state_p = &state;
#endif

    lv_tlsf_walk_pool(lv_tlsf_get_pool(state_p->tlsf), lv_mem_walker, mon_p);

    mon_p->total_size = LV_MEM_SIZE;
    mon_p->used_pct = 100 - (100U * mon_p->free_size) / mon_p->total_size;
    if(mon_p->free_size > 0) {
        mon_p->frag_pct = mon_p->free_biggest_size * 100U / mon_p->free_size;
        mon_p->frag_pct = 100 - mon_p->frag_pct;
    }
    else {
        mon_p->frag_pct = 0; /*no fragmentation if all the RAM is used*/
    }

    mon_p->max_used = state_p->max_used;

    MEM_TRACE("finished");
#endif
}

uint32_t _lv_mem_get_base(uint32_t* size)
{
//    if (size) {
//        *size = LV_MEM_SIZE;
//    }
//    return (uint32_t)lv_tlsf_get_pool(tlsf);
    if (size) {
        *size = 0;
    }
    return 0;
}

/**
 * Give the size of an allocated memory
 * @param data pointer to an allocated memory
 * @return the size of data memory in bytes
 */

#if LV_ENABLE_GC == 0

uint32_t _lv_mem_get_size(const void * data)
{
    if(data == NULL) return 0;
    if(data == &zero_mem) return 0;

    return lv_tlsf_block_size((void *)data);
}

#else /* LV_ENABLE_GC */

uint32_t _lv_mem_get_size(const void * data)
{
    return LV_MEM_CUSTOM_GET_SIZE(data);
}

#endif /*LV_ENABLE_GC*/

/**
 * Get a temporal buffer with the given size.
 * @param size the required size
 */
void * _lv_mem_buf_get(uint32_t size)
{
    if(size == 0) return NULL;
    
    MEM_TRACE("begin, getting %d bytes", size);

    /*Try to find a free buffer with suitable size */
    int8_t i_guess = -1;
    for(uint8_t i = 0; i < LV_MEM_BUF_MAX_NUM; i++) {
        if(LV_GC_ROOT(_lv_mem_buf[i]).used == 0 && LV_GC_ROOT(_lv_mem_buf[i]).size >= size) {
            if(LV_GC_ROOT(_lv_mem_buf[i]).size == size) {
                LV_GC_ROOT(_lv_mem_buf[i]).used = 1;
                return LV_GC_ROOT(_lv_mem_buf[i]).p;
            }
            else if(i_guess < 0) {
                i_guess = i;
            }
            /*If size of `i` is closer to `size` prefer it*/
            else if(LV_GC_ROOT(_lv_mem_buf[i]).size < LV_GC_ROOT(_lv_mem_buf[i_guess]).size) {
                i_guess = i;
            }
        }
    }

    if(i_guess >= 0) {
        LV_GC_ROOT(_lv_mem_buf[i_guess]).used = 1;
        MEM_TRACE("returning already allocated buffer (buffer id: %d, address: %p)", i_guess,
                  LV_GC_ROOT(lv_mem_buf[i_guess]).p);
        return LV_GC_ROOT(_lv_mem_buf[i_guess]).p;
    }

    /*Reallocate a free buffer*/
    for(uint8_t i = 0; i < LV_MEM_BUF_MAX_NUM; i++) {
        if(LV_GC_ROOT(_lv_mem_buf[i]).used == 0) {
            /*if this fails you probably need to increase your LV_MEM_SIZE/heap size*/
            void * buf = lv_mem_realloc(LV_GC_ROOT(_lv_mem_buf[i]).p, size);
            if(buf == NULL) {
                LV_DEBUG_ASSERT(false, "Out of memory, can't allocate a new buffer (increase your LV_MEM_SIZE/heap size)", 0x00);
                return NULL;
            }
            LV_GC_ROOT(_lv_mem_buf[i]).used = 1;
            LV_GC_ROOT(_lv_mem_buf[i]).size = size;
            LV_GC_ROOT(_lv_mem_buf[i]).p    = buf;
            MEM_TRACE("allocated (buffer id: %d, address: %p)", i, LV_GC_ROOT(lv_mem_buf[i]).p);
            return LV_GC_ROOT(_lv_mem_buf[i]).p;
        }
    }

    LV_DEBUG_ASSERT(false, "No free buffer. Increase LV_MEM_BUF_MAX_NUM.", 0x00);
    return NULL;
}

/**
 * Release a memory buffer
 * @param p buffer to release
 */
void _lv_mem_buf_release(void * p)
{
    MEM_TRACE("begin (address: %p)", p);

    for(uint8_t i = 0; i < LV_MEM_BUF_MAX_NUM; i++) {
        if(LV_GC_ROOT(_lv_mem_buf[i]).p == p) {
            LV_GC_ROOT(_lv_mem_buf[i]).used = 0;
            return;
        }
    }

    LV_LOG_ERROR("lv_mem_buf_release: p is not a known buffer")
}

/**
 * Free all memory buffers
 */
void _lv_mem_buf_free_all(void)
{
    for(uint8_t i = 0; i < LV_MEM_BUF_MAX_NUM; i++) {
        if(LV_GC_ROOT(_lv_mem_buf[i]).p) {
            lv_mem_free(LV_GC_ROOT(_lv_mem_buf[i]).p);
            LV_GC_ROOT(_lv_mem_buf[i]).p = NULL;
            LV_GC_ROOT(_lv_mem_buf[i]).used = 0;
            LV_GC_ROOT(_lv_mem_buf[i]).size = 0;
        }
    }
}

void _lv_mem_set_deceleration(uint8_t deceleration)
{
    mem_deceleration = deceleration;
}

#if LV_MEMCPY_MEMSET_STD 
void* _lv_memcpy(void* dst, const void* src, size_t len)
{
    if (mem_deceleration == 0) {
        return memcpy(dst, src, len);
    }
    else if(mem_deceleration == 1) {
        for (size_t i = 0; i < len; i++) {
            ((uint8_t *)dst)[i] = ((uint8_t*)src)[i];
        }
    }
    else {
        for (size_t i = 0; i < len; i++) {
            ((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
            DECELERATE(mem_deceleration - 1);
        }
    }
    return dst;
}

void _lv_memset(void* dst, uint8_t v, size_t len)
{
    if (mem_deceleration == 0) {
        memset(dst, v, len);
    }
    else if (mem_deceleration == 1) {
        for (size_t i = 0; i < len; i++) {
            ((uint8_t*)dst)[i] = v;
        }
    }
    else {
        for (size_t i = 0; i < len; i++) {
            ((uint8_t*)dst)[i] = v;
            DECELERATE(mem_deceleration - 1);
        }
    }
}
#endif

#if LV_MEMCPY_MEMSET_STD == 0
/**
 * Same as `memcpy` but optimized for 4 byte operation.
 * @param dst pointer to the destination buffer
 * @param src pointer to the source buffer
 * @param len number of byte to copy
 */
LV_ATTRIBUTE_FAST_MEM void * _lv_memcpy(void * dst, const void * src, size_t len)
{
    uint8_t * d8 = dst;
    const uint8_t * s8 = src;

    lv_uintptr_t d_align = (lv_uintptr_t)d8 & ALIGN_MASK;
    lv_uintptr_t s_align = (lv_uintptr_t)s8 & ALIGN_MASK;

    /*Byte copy for unaligned memories*/
    if(s_align != d_align) {
        while(len > 32) {
            REPEAT8(COPY8);
            REPEAT8(COPY8);
            REPEAT8(COPY8);
            REPEAT8(COPY8);
            len -= 32;
        }
        while(len) {
            COPY8
            len--;
        }
        return dst;
    }

    /*Make the memories aligned*/
    if(d_align) {
        d_align = ALIGN_MASK + 1 - d_align;
        while(d_align && len) {
            COPY8;
            d_align--;
            len--;
        }
    }

    uint32_t * d32 = (uint32_t *)d8;
    const uint32_t * s32 = (uint32_t *)s8;
    while(len > 32) {
        REPEAT8(COPY32)
        len -= 32;
    }

    while(len > 4) {
        COPY32;
        len -= 4;
    }

    d8 = (uint8_t *)d32;
    s8 = (const uint8_t *)s32;
    while(len) {
        COPY8
        len--;
    }

    return dst;
}

/**
 * Same as `memset` but optimized for 4 byte operation.
 * @param dst pointer to the destination buffer
 * @param v value to set [0..255]
 * @param len number of byte to set
 */
LV_ATTRIBUTE_FAST_MEM void _lv_memset(void * dst, uint8_t v, size_t len)
{

    uint8_t * d8 = (uint8_t *) dst;

    uintptr_t d_align = (lv_uintptr_t) d8 & ALIGN_MASK;

    /*Make the address aligned*/
    if(d_align) {
        d_align = ALIGN_MASK + 1 - d_align;
        while(d_align && len) {
            SET8(v);
            len--;
            d_align--;
        }
    }

    uint32_t v32 = v + (v << 8) + (v << 16) + (v << 24);

    uint32_t * d32 = (uint32_t *)d8;

    while(len > 32) {
        REPEAT8(SET32(v32));
        len -= 32;
    }

    while(len > 4) {
        SET32(v32);
        len -= 4;
    }

    d8 = (uint8_t *)d32;
    while(len) {
        SET8(v);
        len--;
    }
}

/**
 * Same as `memset(dst, 0x00, len)` but optimized for 4 byte operation.
 * @param dst pointer to the destination buffer
 * @param len number of byte to set
 */
LV_ATTRIBUTE_FAST_MEM void _lv_memset_00(void * dst, size_t len)
{
    uint8_t * d8 = (uint8_t *) dst;
    uintptr_t d_align = (lv_uintptr_t) d8 & ALIGN_MASK;

    /*Make the address aligned*/
    if(d_align) {
        d_align = ALIGN_MASK + 1 - d_align;
        while(d_align && len) {
            SET8(0);
            len--;
            d_align--;
        }
    }

    uint32_t * d32 = (uint32_t *)d8;
    while(len > 32) {
        REPEAT8(SET32(0));
        len -= 32;
    }

    while(len > 4) {
        SET32(0);
        len -= 4;
    }

    d8 = (uint8_t *)d32;
    while(len) {
        SET8(0);
        len--;
    }
}

/**
 * Same as `memset(dst, 0xFF, len)` but optimized for 4 byte operation.
 * @param dst pointer to the destination buffer
 * @param len number of byte to set
 */
LV_ATTRIBUTE_FAST_MEM void _lv_memset_ff(void * dst, size_t len)
{
    uint8_t * d8 = (uint8_t *) dst;
    uintptr_t d_align = (lv_uintptr_t) d8 & ALIGN_MASK;

    /*Make the address aligned*/
    if(d_align) {
        d_align = ALIGN_MASK + 1 - d_align;
        while(d_align && len) {
            SET8(0xFF);
            len--;
            d_align--;
        }
    }

    uint32_t * d32 = (uint32_t *)d8;
    while(len > 32) {
        REPEAT8(SET32(0xFFFFFFFF));
        len -= 32;
    }

    while(len > 4) {
        SET32(0xFFFFFFFF);
        len -= 4;
    }

    d8 = (uint8_t *)d32;
    while(len) {
        SET8(0xFF);
        len--;
    }
}

#endif /*LV_MEMCPY_MEMSET_STD*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if LV_MEM_CUSTOM == 0
static void lv_mem_walker(void * ptr, size_t size, int used, void * user)
{
    LV_UNUSED(ptr);

    lv_mem_monitor_t * mon_p = user;
    if(used) {
        mon_p->used_cnt++;
    }
    else {
        mon_p->free_cnt++;
        mon_p->free_size += size;
        if(size > mon_p->free_biggest_size)
            mon_p->free_biggest_size = size;
    }
}
#endif
