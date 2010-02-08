#include "3dc.h"
#include "bmp2.h"
#include "endianio.h"
#include <String.hpp>
#include "scrshot.hpp"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "ourasert.h"

#include "frontend\avp_menus.h"
extern "C"{
// extern DDPIXELFORMAT DisplayPixelFormat;
 extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
 extern int VideoModeTypeScreen;
 extern unsigned char TestPalette[];
 extern unsigned char KeyboardInput[];
 extern unsigned char DebouncedKeyboardInput[];
 extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
 extern MODULE* playerPherModule;	
};

typedef VOID (*PutWord_F)(WORD, FILE *);
typedef VOID (*PutDword_F)(DWORD, FILE *);

extern void LoadModuleData();

void LogCameraPosForModuleLinking()
{
	if(!playerPherModule) return;
	if(!playerPherModule->name) return;
	
	char Filename[100]={"avp_rifs\\"};
	
	strcat(Filename,Env_List[AvP.CurrentEnv]->main);
	strcat(Filename,".mlf");

	FILE* file = avp_fopen(Filename,"ab");
	if(!file)return;

	char output_buffer[300];
	int length=0;
	
	strcpy(output_buffer,playerPherModule->name);
	length+=(strlen(playerPherModule->name)+4)&~3;

	*(VECTORCH*)&output_buffer[length]=playerPherModule->m_world;
	length+=sizeof(VECTORCH);
	*(MATRIXCH*)&output_buffer[length]=Global_VDB_Ptr->VDB_Mat;
	length+=sizeof(MATRIXCH);
	*(VECTORCH*)&output_buffer[length]=Global_VDB_Ptr->VDB_World;
	length+=sizeof(VECTORCH);
	
	if(length%4 !=0)
	{
		GLOBALASSERT(0);
	}

	fwrite(&output_buffer[0],4,length/4,file);
	fclose(file);		
	textprint("Saving camera for module links");
}
int SaveCameraPosKeyPressed=0;
static BOOL ModuleLinkAssist=FALSE;

void HandleScreenShot()
{
	#ifdef AVP_DEBUG_VERSION

	if (DebouncedKeyboardInput[KEY_F8])
		ScreenShot();

	if (KeyboardInput[KEY_F7])
	{
		if(!SaveCameraPosKeyPressed)
		{
			if(KeyboardInput[KEY_LEFTSHIFT]||KeyboardInput[KEY_RIGHTSHIFT])
			{
				ModuleLinkAssist=TRUE;
				DeleteFile("avp_rifs\\module.aaa");
			}
			else
			{
				LogCameraPosForModuleLinking();
				SaveCameraPosKeyPressed=1;
			}
		}
	}
	else
		SaveCameraPosKeyPressed=0;
	
	if(AvP.MainLoopRunning && ModuleLinkAssist)LoadModuleData();

	#endif
}

extern void CreateScreenShotImage();
					 
void ScreenShot()
{
	/* in d3_func.cpp */
	CreateScreenShotImage();
#if 0
	// this is called via the console by typing SCREENSHOT
	// it'll have the console in the screen shot as a result..
	LPDIRECT3DSURFACE9 front_buffer;

	d3d.lpD3DDevice->CreateImageSurface(ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height, D3DFMT_A8R8G8B8, &front_buffer);

	if(FAILED(d3d.lpD3DDevice->GetFrontBuffer(front_buffer))) {
		OutputDebugString("\n Couldn't get a copy of the front buffer");
		front_buffer->Release();
		return;
	}

	char buf[100];
	sprintf(buf,"avp_screenshot_%d.bmp", screen_num);

	if(FAILED(D3DXSaveSurfaceToFileA(buf, D3DXIFF_BMP, front_buffer, NULL, NULL))) {
		OutputDebugString("\n Save Surface to file failed!!!");
	}
	OutputDebugString("\n WROTE A SCREENSHOT");

	front_buffer->Release();
#endif
}


