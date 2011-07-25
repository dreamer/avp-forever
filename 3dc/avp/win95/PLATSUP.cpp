#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "gameplat.h"
#include "gamedef.h"
#include "dynblock.h"
#include "dynamics.h"
#define UseLocalAssert FALSE
#include "ourasert.h"

extern unsigned char KeyboardInput[];
extern unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern  unsigned char KeyASCII;

/*

  Real PC control functions

*/

bool IDemandLookUp()
{
	return false;
}

bool IDemandLookDown()
{
	return false;
}

bool IDemandTurnLeft()
{
	if (KeyboardInput[KEY_LEFT] || KeyboardInput[KEY_JOYSTICK_BUTTON_15])
	{
		return true;
	}
	return false;
}

bool IDemandTurnRight()
{
	if (KeyboardInput[KEY_RIGHT] || KeyboardInput[KEY_JOYSTICK_BUTTON_16])
	{
		return true;
	}
	return false;
}

bool IDemandGoForward()
{
	if (KeyboardInput[KEY_UP] || KeyboardInput[KEY_JOYSTICK_BUTTON_13])
	{
		return true;
	}
	return false;
}

bool IDemandGoBackward()
{
	if (KeyboardInput[KEY_DOWN] || KeyboardInput[KEY_JOYSTICK_BUTTON_14])
	{
		return true;
	}
	return false;
}

bool IDemandJump()
{
	if (KeyboardInput[KEY_CAPS])
	{
		return true;
	}
	return false;
}

bool IDemandCrouch()
{
	if (KeyboardInput[KEY_Z])
	{
		return true;
	}
	return false;
}

bool IDemandSelect()
{
	if (KeyboardInput[KEY_CR])
		return true;

	if (KeyboardInput[KEY_SPACE]) 
		return true;

	if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_2])
		return true; // bjd - A button?
	else 
		return false;
}

bool IDemandStop()
{
	return false;
}

bool IDemandFaster()
{
	if (KeyboardInput[KEY_LEFTSHIFT])
	{
		return true;
	}
	return false;
}


bool IDemandSideStep()
{
	if (KeyboardInput[KEY_LEFTALT])
	{
		return true;
	}
	return false;
}

bool IDemandPickupItem()
{
	if (KeyboardInput[KEY_P])
	{
		return true;
	}
	return false;
}

bool IDemandDropItem()
{
	if (KeyboardInput[KEY_D])
	{
		return true;
	}
	return false;
}

bool IDemandMenu()
{
	if (KeyboardInput[KEY_M])
	{
		return true;
	}
	return false;
}

bool IDemandOperate()
{
	if (KeyboardInput[KEY_SPACE])
	{
		return true;
	}
	return false;
}

bool IDemandFireWeapon()
{
	if (KeyboardInput[KEY_CR])
	{
		return true;
	}
	return false;
}

/* KJL 11:29:12 10/07/96 - added by me */
bool IDemandPreviousWeapon()
{
	if (KeyboardInput[KEY_1])
	{
		return TRUE;
	}
	return false;
}

bool IDemandNextWeapon()
{
	if (KeyboardInput[KEY_2])
	{
		return TRUE;
	}
	return false;
}

int IDemandChangeEnvironment()
{
	if (KeyboardInput[KEY_F1])
		return 0;
	else if (KeyboardInput[KEY_F2])
		return 1;
	else if (KeyboardInput[KEY_F3])
		return 2;
	else if (KeyboardInput[KEY_F4])
		return 3;
	else if (KeyboardInput[KEY_F5])
		return 4;
	else
		return -1;
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
