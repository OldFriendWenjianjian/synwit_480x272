#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "peripheral_tp.h"
#include "synwit_ui_framework/synwit_ui.h"

void (*fp_tp_read_points)(void);

extern void tp_gt9x_init(const i2c_descriptor_t *desc);
extern void tp_gt9x_read_points(void);

extern void tp_ft6336_init(const i2c_descriptor_t *desc);
extern void tp_ft6336_read_points(void);

extern void tp_ft5426_init(const i2c_descriptor_t *desc);
extern void tp_ft5426_read_points(void);

extern void tp_cst826_init(const i2c_descriptor_t *desc);
extern void tp_cst826_read_points(void);

extern void tp_cst3xx_init(const i2c_descriptor_t *desc);
extern void tp_cst3xx_read_points(void);

tp_dev_t tp_dev;

void peripheral_tp_init(const i2c_descriptor_t *i2c_desc)
{
    const char *module_name = synwit_ui_manifest_get_string("peripheral.tp.driver", "gt9x");
    
    if(strcmp(module_name, "gt9x") == 0) {
        tp_gt9x_init(i2c_desc);
        fp_tp_read_points = tp_gt9x_read_points;
        
    } else if(strcmp(module_name, "ft6336") == 0) {
        tp_ft6336_init(i2c_desc);
        fp_tp_read_points = tp_ft6336_read_points;
        
    } else if(strcmp(module_name, "ft5426") == 0) {
        tp_ft5426_init(i2c_desc);
        fp_tp_read_points = tp_ft5426_read_points;
        
    } else if(strcmp(module_name, "cst826") == 0) {
        tp_cst826_init(i2c_desc);
        fp_tp_read_points = tp_cst826_read_points;
        
    } else if(strcmp(module_name, "cst328") == 0 ||
                strcmp(module_name, "cst3240") == 0) {
        tp_cst3xx_init(i2c_desc);
        fp_tp_read_points = tp_cst3xx_read_points;
        
    } else {
        printf("Unknown tp driver module '%s'\n", module_name);
    }
}

