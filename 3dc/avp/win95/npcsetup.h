#ifndef _included_npcsetup_h_
#define _included_npcsetup_h_

#include "chunk_load.h" /* for RIFFHANDLE type */

/* pass handle to environment rif */
void InitNPCs(RIFFHANDLE);

/* unload them all after intance of game */
void EndNPCs();

#endif /* ! _included_npcsetup_h_ */
