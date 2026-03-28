@echo off

SET dir=src\apps\ui_src\appkit
SET app_c=%dir%\app.c
SET data_dir=%dir%\data
SET data_c=%data_dir%\ui_data00.c

if not exist %app_c% (
	echo "Creating default app.c"
	md %dir%
	cd.>%app_c%
	echo #include "serial_display/serial_display_process.h">>%app_c%
	echo #include "synwit_ui_framework/synwit_ui_internal.h">>%app_c%
	echo.>>%app_c%
	echo synwit_sdisp_ops_t g_sdisp_ops = {>> %app_c%
	echo     .rx_handler = sdisp_handler,>>%app_c%
	echo     .notify = sdisp_notify,>>%app_c%
	echo };>>%app_c%
)

if not exist %data_c% (
	echo "Creating default ui_data.c"
	md %data_dir%

	echo const __attribute__^(^(aligned^(4^)^)^) __attribute__^(^(section^(".SPIFLASH0"^)^)^) unsigned char ui_data0[] = {0xff};>%data_dir%\ui_data00.c
	cd.>%data_dir%\ui_data01.c
	cd.>%data_dir%\ui_data02.c
	cd.>%data_dir%\ui_data03.c
	cd.>%data_dir%\ui_data04.c
	cd.>%data_dir%\ui_data05.c
	cd.>%data_dir%\ui_data06.c
	cd.>%data_dir%\ui_data07.c
)