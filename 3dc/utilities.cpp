
#include "utilities.h"
#include <string>

extern "C" {

FILE *avp_fopen(const char *fileName, const char *mode)
{
	int blah;
	FILE *theFile = 0;
	std::string finalPath;
#ifdef _XBOX
	finalPath.append("d:\\");
	finalPath.append(fileName);
//	return fopen(finalPath.c_str(), mode);
	theFile = fopen(finalPath.c_str(), mode);

	if (!theFile)
	{
		blah = 0;
	}
	return theFile;
#endif
#ifdef WIN32
	return fopen(fileName, mode);
#endif
}



};