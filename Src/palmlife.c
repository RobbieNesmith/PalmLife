#include "palmlife.h"
#include <PalmOS.h>
#include <System/SystemPublic.h>
#include <UI/UIPublic.h>
#include <stdarg.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 160
#define WIDTH 40
#define HEIGHT 30
#define YOFFSET 15
#define PIXEL_SIZE 4

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 err;
	EventType e;
	FormType *pfrm;
	Boolean appActive = true;
	Boolean running = false;
	
	WinHandle screenBufferH;
	WinHandle oldDrawWinH;
	RectangleType sourceBounds;
	RectangleType pixelRect;
	
	Boolean *lifeGrid;
	Boolean *lifeTemp;
	
	// Double buffering code from Noiz
	
	/**
	 * create off screen buffer
	 */
	void createScreenBuffer() {
		UInt16 error;
		WinHandle oldDrawWinH;

		screenBufferH = WinCreateOffscreenWindow
			(WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE, screenFormat, &error);
		sourceBounds.topLeft.x = sourceBounds.topLeft.y = 0;
		sourceBounds.extent.x = WIDTH * PIXEL_SIZE;
		sourceBounds.extent.y = HEIGHT * PIXEL_SIZE;
		oldDrawWinH = WinGetDrawWindow();
		WinSetDrawWindow(screenBufferH);
		WinEraseRectangle(&sourceBounds, 0);
		WinSetDrawWindow(oldDrawWinH);
	}

	void startDrawOffscreen() {
		oldDrawWinH = WinGetDrawWindow();
		WinSetDrawWindow(screenBufferH);
	}

	void endDrawOffscreen() {
		WinSetDrawWindow(oldDrawWinH);
	}

	void flipDisplay() {
		WinCopyRectangle
			(screenBufferH, 0, &sourceBounds, 0, YOFFSET, winPaint);
	}
	
	// End double buffering code from Noiz
	
	int lazyMod(int a, int b) {
		if (a >= b)
			return a-b;
		if (a < 0)
			return a + b;
		return a;
	}
	
	void playPause() {
		running = !running;
	}
	
	void initGrids() {
		lifeGrid = MemPtrNew(sizeof(Boolean) * WIDTH * HEIGHT);
		lifeTemp = MemPtrNew(sizeof(Boolean) * WIDTH * HEIGHT);
	}
	
	void deInitGrids() {
		MemPtrFree(lifeGrid);
		MemPtrFree(lifeTemp);
	}
	
	void randomizeGrid() {
		for (int i = 0; i < WIDTH * HEIGHT; i++) {
			lifeGrid[i] = SysRandom(0) & 1;
			lifeTemp[i] = false;
		}
	}
	
	void eraseGrid() {
		for (int i = 0; i < WIDTH * HEIGHT; i++) {
			lifeGrid[i] = false;
			lifeTemp[i] = false;
		}
	}
	
	void setGridPixel(int x, int y, Boolean value) {
		lifeGrid[x + y * WIDTH] = value;
	}
	
	void copyTempToGrid() {
		for (int i = 0; i < WIDTH * HEIGHT; i++) {
			lifeGrid[i] = lifeTemp[i];
		}
	}
	
	void simulateGrid() {
		for (int idx = 0; idx < WIDTH * HEIGHT; idx++) {
			int x = idx % WIDTH;
			int y = idx / WIDTH;
			int neighbors = 0;
			// scan the 3x3 area around this pixel
			for (int scanX = -1; scanX < 2; scanX++) {
				for (int scanY = -1; scanY < 2; scanY++) {
					int checkingX = lazyMod(x + scanX, WIDTH);
					int checkingY = lazyMod(y + scanY, HEIGHT);
					// if we're looking at this pixel, don't increment neighbors
					if (!(scanX == 0 && scanY == 0)
						&& lifeGrid[checkingX + checkingY * WIDTH]) {
						neighbors++;
					}
				}
			}
			// Life Rule: B3S23
			if ((neighbors == 2 && lifeGrid[idx]) || neighbors == 3) {
				lifeTemp[idx] = true;
			} else {
				lifeTemp[idx] = false;
			}
		}
		copyTempToGrid();
	}
	
	void renderGrid() {
		startDrawOffscreen();
		WinEraseRectangle(&sourceBounds, 0);
		for (int i = 0; i < WIDTH * HEIGHT; i++) {
			Int16 x = i % WIDTH;
			Int16 y = i / WIDTH;
			pixelRect.topLeft.x = PIXEL_SIZE * x;
			pixelRect.topLeft.y = PIXEL_SIZE * y;
			pixelRect.extent.x = PIXEL_SIZE;
			pixelRect.extent.y = PIXEL_SIZE;
			if (lifeGrid[i]) {
				WinDrawRectangle(&pixelRect, 0);
			}
		}
		endDrawOffscreen();
		flipDisplay();
	}

	// Make sure only react to NormalLaunch, not Reset, Beam, Find, GoTo...
	if (cmd == sysAppLaunchCmdNormalLaunch)
	{
		initGrids();
		createScreenBuffer();
		randomizeGrid();

		FrmGotoForm(Form1);

	    while(appActive) 
		{
			EvtGetEvent(&e, 10);
			if (SysHandleEvent(&e)) 
				continue;
			if (MenuHandleEvent((void *)0, &e, &err)) 
				continue;
	
			switch (e.eType) 
			{
				case ctlSelectEvent:
					switch (e.data.ctlSelect.controlID) {
						case Clear:
							eraseGrid();
							renderGrid();
							break;
						case Reset:
							randomizeGrid();
							renderGrid();
							break;
						case Step:
							simulateGrid();
							renderGrid();
							break;
						case Run:
							playPause();
							break;
						case BackButton:
							FrmGotoForm(Form1);
							break;
					}
					break;
			
				case frmLoadEvent:
					FrmSetActiveForm(FrmInitForm(e.data.frmLoad.formID));
					break;
		
				case frmOpenEvent:
					pfrm = FrmGetActiveForm();
					FrmDrawForm(pfrm);
					if (e.data.frmOpen.formID == Form1) {
						renderGrid();
					}
					break;

				case menuEvent:
					switch(e.data.menu.itemID) {
						case MainPatternNewCmd:
							eraseGrid();
							renderGrid();
							break;
						case MainPatternRandomizeCmd:
							randomizeGrid();
							renderGrid();
							break;
						case MainPatternOpenCmd:
							break;
						case MainPatternSaveCmd:
							break;
						
						case MainEditCutCmd:
							break;
						case MainEditCopyCmd:
							break;
						case MainEditPasteCmd:
							break;
						case MainEditSelectAllCmd:
							break;
						
						case MainHelpHelpCmd:
							break;
						case MainHelpAboutCmd:
							running = false;
							FrmGotoForm(FormAbout);
							break;
					}
					break;
				
				case menuOpenEvent:
					running = false;
					break;
				
				case appStopEvent:
					deInitGrids();
					appActive = false;
					break;
				default:
					if (FrmGetActiveForm()) {
						FrmHandleEvent(FrmGetActiveForm(), &e);
					}
			}
			
			if (running) {
				simulateGrid();
				renderGrid();
			}
		}
		FrmCloseAllForms();
	}
	return 0;
}

UInt32 __attribute__((section(".vectors"))) __Startup__(void)
{
	SysAppInfoPtr appInfoP;
	void *prevGlobalsP;
	void *globalsP;
	UInt32 ret;
	
	SysAppStartup(&appInfoP, &prevGlobalsP, &globalsP);
	ret = PilotMain(appInfoP->cmd, appInfoP->cmdPBP, appInfoP->launchFlags);
	SysAppExit(appInfoP, prevGlobalsP, globalsP);
	
	return ret;
}
