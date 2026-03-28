#include <stdio.h>
#include "synwit_ui_framework/synwit_ui.h"
#include "data_provider.h"

#define DEFAULT_UI_DATA_DIR     "S:spi:/"

int data_provider_fs_read(uint32_t offset, void *buf, uint32_t size, uint32_t always_zero)
{
    static lv_fs_file_t uibin_file = {0};
    static bool uibin_opened = false;
    
    lv_fs_res_t fs_res;
    
    // Open ui.bin only once
    if(!uibin_opened) {
        // Read 'ui.data_dir' from SynwitManifest.cfg,
        // if SynwitManifest.cfg does not exist or 
        // the field 'ui.data_dir' is not configured, 
        // use DEFAULT_UI_DATA_DIR as the UI directory.
        const char *ui_dir = 
            synwit_ui_manifest_get_string("ui.data_dir", DEFAULT_UI_DATA_DIR);
        
        // Open ui.bin in the UI directory
        char uibin_path[257];
        snprintf(uibin_path, sizeof(uibin_path), "%s/ui.bin", ui_dir);
        lv_fs_res_t fs_res = lv_fs_open(&uibin_file, uibin_path, LV_FS_MODE_RD);
        if(fs_res != LV_FS_RES_OK) {
            printf("Couldn't open %s\n", uibin_path);
            return -1;
        }
        uibin_opened = true;
    }
    
    // Seek
    uint32_t cur_pos;
    lv_fs_tell(&uibin_file, &cur_pos);
    if(cur_pos != offset) {
        lv_fs_seek(&uibin_file, offset, LV_FS_SEEK_SET);
    }
    
    // Read
    uint32_t br = 0;
    fs_res = lv_fs_read(&uibin_file, buf, size, &br);
    if(fs_res != LV_FS_RES_OK) {
        return -1;
    }
    return br;
}
