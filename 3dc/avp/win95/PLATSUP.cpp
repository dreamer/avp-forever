/*

  Platform specific project specific C functions

*/

#include "3dc.h"
#include "module.h"
#include "inline.h"

#include "gameplat.h"
#include "gamedef.h"

#include "dynblock.h"
#include "dynamics.h"
#define UseLocalAssert FALSE
#include "ourasert.h"

/*
	Externs from pc\io.c
*/

extern unsigned char KeyboardInput[];
extern unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern  unsigned char KeyASCII;

// Prototypes

int IDemandFireWeapon(void);
int IDemandNextWeapon(void);
int IDemandPreviousWeapon(void);


// Functions



void catpathandextension(char* dst, char* src)
{
	int len = lstrlen(dst);

	if ((len > 0 && (dst[len-1] != '\\' && dst[len-1] != '/')) && *src != '.')
		{
			lstrcat(dst,"\\");
		}

    lstrcat(dst,src);

/*
	The second null here is to support the use
	of SHFileOperation, which is a Windows 95
	addition that is uncompilable under Watcom
	with ver 10.5, but will hopefully become
	available later...
*/
    len = lstrlen(dst);
    dst[len+1] = 0;

}


/* game platform definition of the Mouse Mode*/

int MouseMode = MouseVelocityMode;

/*

  Real PC control functions

*/

int IDemandLookUp(void)
{
	return FALSE;
}


int IDemandLookDown(void)
{
	return FALSE;
}

int IDemandTurnLeft(void)
{
	if (KeyboardInput[KEY_LEFT] || KeyboardInput[KEY_JOYSTICK_BUTTON_15])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandTurnRight(void)
{
	if (KeyboardInput[KEY_RIGHT] || KeyboardInput[KEY_JOYSTICK_BUTTON_16])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandGoForward(void)
{
	if (KeyboardInput[KEY_UP] || KeyboardInput[KEY_JOYSTICK_BUTTON_13])
	{
		return TRUE;
	}
	return FALSE;
}


int IDemandGoBackward(void)
{
	if (KeyboardInput[KEY_DOWN] || KeyboardInput[KEY_JOYSTICK_BUTTON_14])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandJump(void)
{
	if (KeyboardInput[KEY_CAPS])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandCrouch(void)
{
	if(KeyboardInput[KEY_Z])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandSelect(void)
{
	if (KeyboardInput[KEY_CR])
		return TRUE;

	if (KeyboardInput[KEY_SPACE]) 
		return TRUE;

	if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_2])
		return TRUE; // bjd - A button?
	else 
		return FALSE;
}

int IDemandStop(void)
{
	return FALSE;
}


int IDemandFaster(void)
{
	if (KeyboardInput[KEY_LEFTSHIFT])
	{
		return TRUE;
	}
	return FALSE;
}


int IDemandSideStep(void)
{
	if (KeyboardInput[KEY_LEFTALT])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandPickupItem(void)
{
	if (KeyboardInput[KEY_P])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandDropItem(void)
{
	if (KeyboardInput[KEY_D])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandMenu(void)
{
	if (KeyboardInput[KEY_M])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandOperate(void)
{
	if (KeyboardInput[KEY_SPACE])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandFireWeapon(void)
{
	if (KeyboardInput[KEY_CR])
	{
		return TRUE;
	}
	return FALSE;
}

/* KJL 11:29:12 10/07/96 - added by me */
int IDemandPreviousWeapon(void)
{
	if(KeyboardInput[KEY_1])
	{
		return TRUE;
	}
	return FALSE;
}
int IDemandNextWeapon(void)
{
	if (KeyboardInput[KEY_2])
	{
		return TRUE;
	}
	return FALSE;
}

int IDemandChangeEnvironment()
{
	if(KeyboardInput[KEY_F1])
		return 0;
	else if(KeyboardInput[KEY_F2])
		return 1;
	else if(KeyboardInput[KEY_F3])
		return 2;
	else if(KeyboardInput[KEY_F4])
		return 3;
	else if(KeyboardInput[KEY_F5])
		return 4;
	else
		return(-1);
}


/* KJL 15:53:52 05/04/97 - 
Loaders/Unloaders for language internationalization code in language.c */

char *LoadTextFile(char *filename)
{
	char *bufferPtr;
	long int save_pos, size_of_file;
	FILE *fp;

	fp = avp_fopen(filename,"rb");
	
	if (!fp) goto error;

	save_pos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	size_of_file = ftell(fp);
	
	bufferPtr = (char*)AllocateMem(size_of_file);
	if (!bufferPtr)
	{
		memoryInitialisationFailure = 1;
		goto error;
	}

	fseek(fp,save_pos,SEEK_SET);

	
	if (!fread(bufferPtr, size_of_file,1,fp))
	{
		fclose(fp);
		goto error;
	}
			
	fclose(fp);
	return bufferPtr;
	
error:
	{
		/* error whilst trying to load file */
		textprint("Error! Can not load file %s.\n",filename);
		LOCALASSERT(0);
		return 0;
	}
}


void UnloadTextFile(char *filename, char *bufferPtr)
{
	if (bufferPtr) DeallocateMem(bufferPtr);
}
