#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/moduleinfo.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>

#include "utils.h"
#include "draw.h"
#include "filebrowser.c"

PSP2_MODULE_INFO(0, 0, "CyanogenVita File Manager")

int main()
{
	init_video();

	char * testDirectory = dirBrowse("ms0:");

	for(;;)
	{
		draw_rectangle(SCREEN_W, SCREEN_H, 60, 60, BLACK);
		centerText(960/2, 272/2, testDirectory, 50, WHITE);
		swap_buffers();
		sceDisplayWaitVblankStart();
	}
	
	end_video();
	return 0;
}
