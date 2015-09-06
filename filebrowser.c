#ifndef _FILEBROWSE_C
#define _FILEBROWSE_C

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


#define printf				pspDebugScreenPrintf

#define MAX_FILES			255 // Max amount of files to be loaded
#define MAX_DISPLAY			20 // Max files to be displayed on screen
#define DISPLAY_X			50 // X value browser display
#define DISPLAY_Y			50 // Y value of browser display

//Globals

typedef struct fileIcon {

	int		active;

	char	name[255];
	char	filePath[255];
	char	fileType[255];

	int		x;
	int		y;

} fileIcon;

fileIcon folderIcons[MAX_FILES];

typedef struct File {

	int exist;

	char path[255];
	char name[255];

	int size;
	int directory;

} File;

File dirScan[MAX_FILES];


SceIoDirent g_dir;

int i;
int current;
int curScroll;
char currDir[512];
int timer;
char returnMe[512];
SceCtrlData pad;
SceCtrlData oldpad;

// Function prototypes

int folderScan(char* path);
int runFile( char* path, char* type );
void centerText(int centerX, int centerY, char * centerText, int centerLength, uint32_t color);
void dirVars();
void dirBack();
void dirUp();
void dirDown();
void dirControls();
char * dirBrowse(char * path);

//Functions

int folderScan( char* path )
{
	curScroll = 1;
	sprintf(currDir, path);

	int i;
	for (i=0; i<=MAX_FILES; i++)	// erase old folders
		dirScan[i].exist = 0;

	int x;
	for (x=0; x<=MAX_FILES; x++) {
		folderIcons[x].active = 0;
	}

	int fd = sceIoDopen( path );

	i = 1;
	
	if (fd) {
		if (!(strcmp(path, "cache0:")==0 || (strcmp(path, "cache0:/")==0))) {

			sceIoDread(fd, &g_dir);		// get rid of '.' and '..'
			sceIoDread(fd, &g_dir);

			// Create our own '..'
			folderIcons[1].active = 1; 
			sprintf(folderIcons[1].filePath, "doesn't matter");
			sprintf(folderIcons[1].name, "..");
			sprintf(folderIcons[1].fileType, "dotdot");

			x = 2;
		} else {
			x = 1;
		}
		while ( sceIoDread(fd, &g_dir) && i<=MAX_FILES ) {
			sprintf( dirScan[i].name, g_dir.d_name );
			sprintf( dirScan[i].path, "%s/%s", path, dirScan[i].name );

			if (g_dir.d_stat.st_attr & PSP2_S_IFDIR) {
				dirScan[i].directory = 1;
				dirScan[i].exist = 1;
			} else {
				dirScan[i].directory = 0;
				dirScan[i].exist = 1;
			}

			dirScan[i].size = g_dir.d_stat.st_size;
			i++;
		}
	}

	sceIoDclose(fd);

	for (i=1; i<MAX_FILES; i++) {
		if (dirScan[i].exist == 0) break;
		folderIcons[x].active = 1;
		sprintf(folderIcons[x].filePath, dirScan[i].path);
		sprintf(folderIcons[x].name, dirScan[i].name);

		char *suffix = strrchr(dirScan[i].name, '.');
		
		if (dirScan[i].directory == 1) {      // if it's a directory
			sprintf(folderIcons[x].fileType, "fld");
		} 
		else if ((dirScan[i].directory == 0) && (suffix)) {		// if it's not a directory
			sprintf(folderIcons[x].fileType, "none");
		}
		else if (!(suffix)) {
			sprintf(folderIcons[x].fileType, "none");
		}
		x++;
	}

	return 1;
}

int runFile( char* path, char* type )
{
	// Folders
	if (strcmp(type, "fld")==0) {
		folderScan(path);
	}
	// '..' or 'dotdot'
	else if (strcmp(type, "dotdot")==0){
		dirBack();
	}
	// Other
	else if (strcmp(type, "none")==0){
		sprintf(returnMe, path);
	}

	return 1;
}

// Print Out of Bounds Text
void printTextScreen(char * text, int x, int y, uint32_t color)
{
	// Convert screen coordinates from 480x272 to 960x544
	font_draw_string((x*2*95)/100, y*2, color, text);
}

void centerText(int centerX, int centerY, char * centerText, int centerLength, uint32_t color)
{
	char str[255];

	if (strlen(centerText) <= centerLength) {
		int center = ((centerX)-((strlen(centerText)/2)*8));
		printTextScreen(str, center, centerY, color);
	}
	else {
		int center = ((centerX)-(centerLength/2)*8);
		strncpy(str, centerText, centerLength);
		str[centerLength-3] = '.';
		str[centerLength-2] = '.';
		str[centerLength-1] = '.';
		str[centerLength] = '\0';
		printTextScreen(str, center, centerY, color);
	}
}

void dirVars()
{
	sprintf(currDir, "cache0:");
	sprintf(returnMe, "blah");
	returnMe[5] = '\0';
	current = 1;
	curScroll = 1;
	timer = 0;
}

void dirUp()
{
	current--; // Subtract a value from current so the ">" goes up
	if ((current <= curScroll-1) && (curScroll > 1)) {
		curScroll--;
	}
}

void dirDown()
{
	if (folderIcons[current+1].active) current++; // Add a value onto current so the ">" goes down
	if (current >= (MAX_DISPLAY+curScroll)) {
		curScroll++;
	}
}

void dirDisplay()
{
	draw_rectangle(SCREEN_W, SCREEN_H, 60, 60, BLACK); // Background
	centerText(960/2, 1, currDir, 50, WHITE); // Current directory

	// Displays the directories, while also incorporating the scrolling
	for(i=curScroll;i<MAX_DISPLAY+curScroll;i++) {
		// Handles the ">" and the display to not move past the MAX_DISPLAY
		// For moving down
		//if ((folderIcons[i].active == 0) && (current >= i-1)) {
		if ((folderIcons[i].active == 0) && (current >= i-1)) {
			current = i-1;
			break;
		}
		// For moving up
		if (current <= curScroll-1) {
			current = curScroll-1;
			break;
		}
			
		// If the currently selected item is active, then display the name
		if (folderIcons[i].active == 1) {
			printTextScreen(folderIcons[i].name, DISPLAY_X, (i - curScroll)*10+DISPLAY_Y, RED);
		}
	}
	printTextScreen(">", DISPLAY_X-10, (current - curScroll)*10+DISPLAY_Y, RED);
}

void dirControls()
{
	if (pad.buttons != oldpad.buttons) {
		if ((pad.buttons & PSP2_CTRL_DOWN) && (!(oldpad.buttons & PSP2_CTRL_DOWN))) {
			dirDown();
			timer = 0;
		}
		else if ((pad.buttons & PSP2_CTRL_UP) && (!(oldpad.buttons & PSP2_CTRL_UP))) {
			dirUp();
			timer = 0;
		}
		if ((pad.buttons & PSP2_CTRL_CROSS) && (!(oldpad.buttons & PSP2_CTRL_CROSS))) {
			runFile(folderIcons[current].filePath, folderIcons[current].fileType);
		}
		if ((pad.buttons & PSP2_CTRL_TRIANGLE) && (!(oldpad.buttons & PSP2_CTRL_TRIANGLE))) {
			if (!(strcmp(currDir, "cache0:")==0) || (strcmp(currDir, "cache0:/")==0)) {
				curScroll = 1;
				current = 1;
			}
		}
	}
		
	timer++;
	if ((timer > 30) && (pad.buttons & PSP2_CTRL_UP)) {
		dirUp();
		timer = 25;
	} else if ((timer > 30) && (pad.buttons & PSP2_CTRL_DOWN)) {
		dirDown();
		timer = 25;
	}

	if (current < 1) current = 1; // Stop the ">" from moving past the minimum files
	if (current > MAX_FILES) current = MAX_FILES; // Stop the ">" from moving past the max files

}

void dirBack()
{
	int a = 0;
	int b = 0;
	if (strlen(currDir) > strlen("cache0:/")) {
		for (a=strlen(currDir);a>=0;a--) {
			if (currDir[a] == '/') {
				b++;
			}
			currDir[a] = '\0';
			if (b == 1) {
				break;
			}
		}
		curScroll = 1;
		folderScan(currDir);
	} 
}

char * dirBrowse(char * path)
{
	folderScan(path);
	dirVars();

	for(;;) {
		oldpad = pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);

		dirDisplay();
		dirControls();
		
		if (strlen(returnMe) > 4) {
			break;
		}

		swap_buffers();
		sceDisplayWaitVblankStart();
	}

	return returnMe;
}

#endif