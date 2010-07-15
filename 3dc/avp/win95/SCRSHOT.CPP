#include "3dc.h"
//#include "bmp2.h"
#include "endianio.h"
#include <String.hpp>
#include "scrshot.hpp"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "ourasert.h"
#include "renderer.h"

#include "frontend\avp_menus.h"
extern "C"
{
	extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
	extern int VideoModeTypeScreen;
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
	
	char Filename[MAX_PATH]={"avp_rifs\\"};
	
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

int SaveCameraPosKeyPressed = 0;
static BOOL ModuleLinkAssist = FALSE;

void HandleScreenShot()
{
	#ifdef AVP_DEBUG_VERSION

	if (DebouncedKeyboardInput[KEY_F8])
		ScreenShot();

	if (KeyboardInput[KEY_F7])
	{
		if (!SaveCameraPosKeyPressed)
		{
			if (KeyboardInput[KEY_LEFTSHIFT] || KeyboardInput[KEY_RIGHTSHIFT])
			{
				ModuleLinkAssist = TRUE;
				DeleteFile("avp_rifs\\module.aaa");
			}
			else
			{
				LogCameraPosForModuleLinking();
				SaveCameraPosKeyPressed = 1;
			}
		}
	}
	else
		SaveCameraPosKeyPressed = 0;
	
	if (AvP.MainLoopRunning && ModuleLinkAssist)
		LoadModuleData();

	#endif
}

extern void CreateScreenShotImage();

void ScreenShot()
{
	// in d3_func.cpp
	CreateScreenShotImage();
}
