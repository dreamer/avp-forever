#define _BSD_SOURCE

#include <assert.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#	include "dirent/dirent.h"
#	include <direct.h>
#	include <io.h>
#	ifndef S_ISDIR
#		define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#	endif

#	ifndef S_ISREG
#		define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#	endif

#	ifndef access
#		define access _access
#	endif

#	ifndef R_OK
#		define R_OK    4
#	endif
#	ifndef W_OK
#		define W_OK    2
#	endif
#	ifndef F_OK
#		define F_OK    0
#	endif

#	ifndef mkdir
#		define mkdir( _Filename, _Flags ) _mkdir( ( ( _Filename ) ) )
#	endif

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define	EOS	'\0'
#define FNM_NOMATCH      1     /* Match failed. */
#define FNM_NOSYS        2     /* Function not supported (unused). */

#define FNM_NOESCAPE     0x01        /* Disable backslash escaping. */
#define FNM_PATHNAME     0x02        /* Slash must be matched by slash. */
#define FNM_PERIOD       0x04        /* Period must be matched by period. */
#define FNM_LEADING_DIR  0x08        /* Ignore /<tail> after Imatch. */
#define FNM_CASEFOLD     0x10        /* Case insensitive search. */
#define FNM_PREFIX_DIRS  0x20

static const char *rangematch(const char *, char, int);

int fnmatch(const char *pattern, const char *string, int flags)
{
	const char *stringstart;
	char c, test;

	for (stringstart = string;;)
		switch (c = *pattern++) {
		case EOS:
			if ((flags & FNM_LEADING_DIR) && *string == '/')
				return (0);
			return (*string == EOS ? 0 : FNM_NOMATCH);
		case '?':
			if (*string == EOS)
				return (FNM_NOMATCH);
			if (*string == '/' && (flags & FNM_PATHNAME))
				return (FNM_NOMATCH);
			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				return (FNM_NOMATCH);
			++string;
			break;
		case '*':
			c = *pattern;
			/* Collapse multiple stars. */
			while (c == '*')
				c = *++pattern;

			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				return (FNM_NOMATCH);

			/* Optimize for pattern with * at end or before /. */
			if (c == EOS)
				if (flags & FNM_PATHNAME)
					return ((flags & FNM_LEADING_DIR) ||
					    strchr(string, '/') == NULL ?
					    0 : FNM_NOMATCH);
				else
					return (0);
			else if (c == '/' && flags & FNM_PATHNAME) {
				if ((string = strchr(string, '/')) == NULL)
					return (FNM_NOMATCH);
				break;
			}

			/* General case, use recursion. */
			while ((test = *string) != EOS) {
				if (!fnmatch(pattern, string, flags & ~FNM_PERIOD))
					return (0);
				if (test == '/' && flags & FNM_PATHNAME)
					break;
				++string;
			}
			return (FNM_NOMATCH);
		case '[':
			if (*string == EOS)
				return (FNM_NOMATCH);
			if (*string == '/' && flags & FNM_PATHNAME)
				return (FNM_NOMATCH);
			if ((pattern =
			    rangematch(pattern, *string, flags)) == NULL)
				return (FNM_NOMATCH);
			++string;
			break;
		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				if ((c = *pattern++) == EOS) {
					c = '\\';
					--pattern;
				}
			}
			/* FALLTHROUGH */
		default:
			if (c == *string)
				;
			else if ((flags & FNM_CASEFOLD) &&
				 (tolower((unsigned char)c) ==
				  tolower((unsigned char)*string)))
				;
			else if ((flags & FNM_PREFIX_DIRS) && *string == EOS &&
			     (c == '/' && string != stringstart ||
			     string == stringstart+1 && *stringstart == '/') )
				return (0);
			else
				return (FNM_NOMATCH);
			string++;
			break;
		}
	/* NOTREACHED */
}

static const char *
rangematch(const char *pattern, char test, int flags)
{
	int negate, ok;
	char c, c2;

	/*
	 * A bracket expression starting with an unquoted circumflex
	 * character produces unspecified results (IEEE 1003.2-1992,
	 * 3.13.2).  This implementation treats it like '!', for
	 * consistency with the regular expression syntax.
	 * J.T. Conklin (conklin@ngai.kaleida.com)
	 */
	if ( (negate = (*pattern == '!' || *pattern == '^')) )
		++pattern;

	if (flags & FNM_CASEFOLD)
		test = tolower((unsigned char)test);

	for (ok = 0; (c = *pattern++) != ']';) {
		if (c == '\\' && !(flags & FNM_NOESCAPE))
			c = *pattern++;
		if (c == EOS)
			return (NULL);

		if (flags & FNM_CASEFOLD)
			c = tolower((unsigned char)c);

		if (*pattern == '-'
		    && (c2 = *(pattern+1)) != EOS && c2 != ']') {
			pattern += 2;
			if (c2 == '\\' && !(flags & FNM_NOESCAPE))
				c2 = *pattern++;
			if (c2 == EOS)
				return (NULL);

			if (flags & FNM_CASEFOLD)
				c2 = tolower((unsigned char)c2);

			if ((unsigned char)c <= (unsigned char)test &&
			    (unsigned char)test <= (unsigned char)c2)
				ok = 1;
		} else if (c == test)
			ok = 1;
	}
	return (ok == negate ? NULL : pattern);
}

#else
#include <dirent.h>
#include <fnmatch.h>
#endif

#include "fixer.h"
#include "files.h"

static char *local_dir;
static char *global_dir;

/*
Sets the local and global directories used by the other functions.
Local = ~/.dir, where config and user-installed files are kept.
Global = installdir, where installed data is stored.
*/
int SetGameDirectories(const char *local, const char *global)
{
	struct stat buf;
	
	local_dir = strdup(local);
	global_dir = strdup(global);
	
	if (stat(local_dir, &buf) == -1) {
		printf("Creating local directory %s...\n", local_dir);
		
		mkdir(local_dir, S_IRWXU);
	}
	
	return 0;
}

#define DIR_SEPARATOR	"/"

static char *FixFilename(const char *filename, const char *prefix, int force)
{
	char *f, *ptr;
	int flen;
	int plen;
	
	plen = strlen(prefix) + 1;
	flen = strlen(filename) + plen + 1;
	
	f = (char *)malloc(flen);
	strcpy(f, prefix);
	strcat(f, DIR_SEPARATOR);
	strcat(f, filename);
	
	/* only the filename part needs to be modified */
	ptr = &f[plen+1];
	
	while (*ptr) {
		if ((*ptr == '/') || (*ptr == '\\') || (*ptr == ':')) {
			*ptr = DIR_SEPARATOR[0];
		} else if (*ptr == '\r' || *ptr == '\n') {
			*ptr = 0;
			break;
		} else {
			if (force) {
				*ptr = tolower(*ptr);
			}
		}
		ptr++;
	}

	return f;
}

/*
Open a file of type type, with mode mode.

Mode can be:
#define	FILEMODE_READONLY	0x01
#define	FILEMODE_WRITEONLY	0x02
#define	FILEMODE_READWRITE	0x04
#define FILEMODE_APPEND		0x08
Type is (mode = ReadOnly):
#define	FILETYPE_PERM		0x08 // try the global dir only 
#define	FILETYPE_OPTIONAL	0x10 // try the global dir first, then try the local dir
#define	FILETYPE_CONFIG		0x20 // try the local dir only

Type is (mode = WriteOnly or ReadWrite):
FILETYPE_PERM: error
FILETYPE_OPTIONAL: error
FILETYPE_CONFIG: try the local dir only
*/
FILE *OpenGameFile(const char *filename, int mode, int type)
{
	char *rfilename;
	char *openmode;
	FILE *fp;
	
	if ((type != FILETYPE_CONFIG) && (mode != FILEMODE_READONLY)) 
		return NULL;
	
	switch(mode) {
		case FILEMODE_READONLY:
			openmode = "rb";
			break;
		case FILEMODE_WRITEONLY:
			openmode = "wb";
			break;
		case FILEMODE_READWRITE:
			openmode = "w+";
			break;
		case FILEMODE_APPEND:
			openmode = "ab";
			break;
		default:
			return NULL;
	}

	if (type != FILETYPE_CONFIG) {
		rfilename = FixFilename(filename, global_dir, 0);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		if (fp != NULL) {
			return fp;
		}
		
		rfilename = FixFilename(filename, global_dir, 1);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		if (fp != NULL) {
			return fp;
		}
	}
	
	if (type != FILETYPE_PERM) {
		rfilename = FixFilename(filename, local_dir, 0);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		if (fp != NULL) {
			return fp;
		}
		
		rfilename = FixFilename(filename, local_dir, 1);
		
		fp = fopen(rfilename, openmode);
		
		free(rfilename);
		
		return fp;
	}
	
	return NULL;
}

/*
Close a fd returned from OpenGameFile

Currently this just uses stdio, so CloseGameFile is redundant.
*/
int CloseGameFile(FILE *pfd)
{
	return fclose(pfd);
}


/*
Get the filesystem attributes of a file

#define	FILEATTR_DIRECTORY	0x0100
#define FILEATTR_READABLE	0x0200
#define FILEATTR_WRITABLE	0x0400

Error or can't access it: return value of 0 (What is the game going to do about it anyway?)
*/
static int GetFA(const char *filename)
{
	struct stat buf;
	int attr;

	attr = 0;
	if (stat(filename, &buf) == 0) {
		if (S_ISDIR(buf.st_mode)) {
			attr |= FILEATTR_DIRECTORY;
		}
			
		if (access(filename, R_OK) == 0) {
			attr |= FILEATTR_READABLE;
		}
			
		if (access(filename, W_OK) == 0) {
			attr |= FILEATTR_WRITABLE;
		}
	}
	
	return attr;
}

static time_t GetTS(const char *filename)
{
	struct stat buf;

	if (stat(filename, &buf) == 0) {
		return buf.st_mtime;
	}
	
	return 0;
}

int GetGameFileAttributes(const char *filename, int type)
{
	struct stat buf;
	char *rfilename;
	int attr;
	
	attr = 0;	
	if (type != FILETYPE_CONFIG) {
		rfilename = FixFilename(filename, global_dir, 0);
		
		if (stat(rfilename, &buf) == 0) {
			if (S_ISDIR(buf.st_mode)) {
				attr |= FILEATTR_DIRECTORY;
			}
			
			if (access(rfilename, R_OK) == 0) {
				attr |= FILEATTR_READABLE;
			}
			
			if (access(rfilename, W_OK) == 0) {
				attr |= FILEATTR_WRITABLE;
			}
			
			free(rfilename);
		
			return attr;
		}
		
		free(rfilename);
		
		rfilename = FixFilename(filename, global_dir, 1);
		
		if (stat(rfilename, &buf) == 0) {
			if (S_ISDIR(buf.st_mode)) {
				attr |= FILEATTR_DIRECTORY;
			}
			
			if (access(rfilename, R_OK) == 0) {
				attr |= FILEATTR_READABLE;
			}
			
			if (access(rfilename, W_OK) == 0) {
				attr |= FILEATTR_WRITABLE;
			}
			
			free(rfilename);
		
			return attr;
		}
		
		free(rfilename);
	}
	
	if (type != FILETYPE_PERM) {
		rfilename = FixFilename(filename, local_dir, 0);
		
		if (stat(rfilename, &buf) == 0) {
			if (S_ISDIR(buf.st_mode)) {
				attr |= FILEATTR_DIRECTORY;
			}
			
			if (access(rfilename, R_OK) == 0) {
				attr |= FILEATTR_READABLE;
			}
			
			if (access(rfilename, W_OK) == 0) {
				attr |= FILEATTR_WRITABLE;
			}
			
			free(rfilename);
		
			return attr;
		}
		
		free(rfilename);
		
		rfilename = FixFilename(filename, local_dir, 1);
		
		if (stat(rfilename, &buf) == 0) {
			if (S_ISDIR(buf.st_mode)) {
				attr |= FILEATTR_DIRECTORY;
			}
			
			if (access(rfilename, R_OK) == 0) {
				attr |= FILEATTR_READABLE;
			}
			
			if (access(rfilename, W_OK) == 0) {
				attr |= FILEATTR_WRITABLE;
			}
			
			free(rfilename);
		
			return attr;
		}
		
		free(rfilename);
		
	}
	
	return 0;
}

/*
Delete a file: local dir only
*/
int DeleteGameFile(const char *filename)
{
	char *rfilename;
	int ret;
	
	rfilename = FixFilename(filename, local_dir, 0);
	ret = unlink(rfilename);
	free(rfilename);
	
	if (ret == -1) {
		rfilename = FixFilename(filename, local_dir, 1);
		ret = unlink(rfilename);
		free(rfilename);
	}
	
	return ret;
}

/*
Create a directory: local dir only

TODO: maybe also mkdir parent directories, if they do not exist?
*/
int CreateGameDirectory(const char *dirname)
{
	char *rfilename;
	int ret;

	rfilename = FixFilename(dirname, local_dir, 0);
	ret = mkdir(rfilename, S_IRWXU);
	free(rfilename);
	
	if (ret == -1) {
		rfilename = FixFilename(dirname, local_dir, 1);
		ret = mkdir(rfilename, S_IRWXU);
		free(rfilename);
	}
	
	return ret;
}

/* This struct is private. */
typedef struct GameDirectory
{
	DIR *localdir;		/* directory opened with opendir */
	DIR *globaldir;
	
	char *localdirname;
	char *globaldirname;
	
	char *pat;		/* pattern to match */
	
	GameDirectoryFile tmp;	/* Temp space */
} GameDirectory;

/*
"Open" a directory dirname, with type type
Returns a pointer to a directory datatype

Pattern is the pattern to match
*/
void *OpenGameDirectory(const char *dirname, const char *pattern, int type)
{
	char *localdirname, *globaldirname;
	DIR *localdir, *globaldir;
	GameDirectory *gd;
	
	globaldir = NULL;
	globaldirname = NULL;
	if (type != FILETYPE_CONFIG) {
		globaldirname = FixFilename(dirname, global_dir, 0);
		
		globaldir = opendir(globaldirname);
		
		if (globaldir == NULL) {
			free(globaldirname);
			
			globaldirname = FixFilename(dirname, global_dir, 1);
		
			globaldir = opendir(globaldirname);
		
			if (globaldir == NULL)
				free(globaldirname);
		}		
	}
	
	localdir = NULL;
	localdirname = NULL;
	if (type != FILETYPE_PERM) {
		localdirname = FixFilename(dirname, local_dir, 0);
		
		localdir = opendir(localdirname);
		
		if (localdir == NULL) {
			free(localdirname);
			
			localdirname = FixFilename(dirname, local_dir, 1);
		
			localdir = opendir(localdirname);
		
			if (localdir == NULL)
				free(localdirname);
		}
	}
	
	if (localdir == NULL && globaldir == NULL)
		return NULL;

	gd = (GameDirectory *)malloc(sizeof(GameDirectory));

	gd->localdir = localdir;
	gd->globaldir = globaldir;

	gd->localdirname = localdirname;
	gd->globaldirname = globaldirname;
	
	gd->pat = strdup(pattern);

	return gd;
}

/*
This struct is public.

typedef struct GameDirectoryFile
{
	char *filename;
	int attr;
} GameDirectoryFile;
*/

/*
Returns the next match of pattern with the contents of dir

f is the current file
*/
GameDirectoryFile *ScanGameDirectory(void *dir)
{
	char *ptr;
	struct dirent *file;
	GameDirectory *directory;
	
	directory = (GameDirectory *)dir;
	
	if (directory->globaldir) {
		while ((file = readdir(directory->globaldir)) != NULL) {
			if (fnmatch(directory->pat, file->d_name, FNM_PATHNAME) == 0) {
				ptr = FixFilename(file->d_name, directory->globaldirname, 0);
				directory->tmp.attr = GetFA(ptr);
				free(ptr);
				
				directory->tmp.filename = file->d_name;

				return &directory->tmp;
			}
		}
		closedir(directory->globaldir);
		free(directory->globaldirname);
		
		directory->globaldir = NULL;
		directory->globaldirname = NULL;
	}
	
	if (directory->localdir) {
		while ((file = readdir(directory->localdir)) != NULL) {
			if (fnmatch(directory->pat, file->d_name, FNM_PATHNAME) == 0) {
				ptr = FixFilename(file->d_name, directory->localdirname, 0);
				directory->tmp.attr = GetFA(ptr);
				directory->tmp.timestamp = GetTS(ptr);
				free(ptr);
				
				directory->tmp.filename = file->d_name;
				
				return &directory->tmp;
			}
		}
		closedir(directory->localdir);
		free(directory->localdirname);
		
		directory->localdir = NULL;
		directory->localdirname = NULL;
	}
	
	return NULL;
}

/*
Close directory
*/
int CloseGameDirectory(void *dir)
{
	GameDirectory *directory = (GameDirectory *)dir;
	
	if (directory) {
		free(directory->pat);

		if (directory->localdirname)
			free(directory->localdirname);
		if (directory->globaldirname)
			free(directory->globaldirname);
		if (directory->localdir)
			closedir(directory->localdir);
		if (directory->globaldir)
			closedir(directory->globaldir);
			
		return 0;
	}
	return -1;
}

/*
  Game-specific helper function.
 */
static int try_game_directory(char *dir, char *file)
{
	char tmppath[PATH_MAX];

	strncpy(tmppath, dir, PATH_MAX-32);
	tmppath[PATH_MAX-32] = 0;
	strcat(tmppath, file);
	
	return access(tmppath, R_OK) == 0;
}

/*
  Game-specific helper function.
 */
static int check_game_directory(char *dir)
{
	if (!dir || !*dir) {
		return 0;
	}
	
	if (!try_game_directory(dir, "/avp_huds")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "/avp_huds/alien.rif")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "/avp_rifs")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "/avp_rifs/temple.rif")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "/fastfile")) {
		return 0;
	}
	
	if (!try_game_directory(dir, "/fastfile/ffinfo.txt")) {
		return 0;
	}
	
	return 1;
}

/*
  Game-specific initialization
 */
void InitGameDirectories(char *argv0)
{
	extern char *SecondTex_Directory;
	extern char *SecondSoundDir;
	
	char tmppath[PATH_MAX];
	char *homedir, *gamedir, *localdir, *tmp;
	char *path;
	size_t len, copylen;
	
	SecondTex_Directory = "graphics/";
	SecondSoundDir = "sound/";

	homedir = getenv("HOME");
	if (homedir == NULL)
		homedir = ".";
	localdir = (char *)malloc(strlen(homedir)+10);
	strcpy(localdir, homedir);
	strcat(localdir, "/");
	strcat(localdir, ".avp");
	
	tmp = NULL;
	
	/*
	1. $AVP_DATA overrides all
	2. executable path from argv[0]
	3. realpath of executable path from argv[0]
	4. $PATH
	5. current directory
	*/
	
	/* 1. $AVP_DATA */
	gamedir = getenv("AVP_DATA");
	
	/* $AVP_DATA overrides all, so no check */
	
	if (gamedir == NULL) {
		/* 2. executable path from argv[0] */
		tmp = strdup(argv0);
		
		if (tmp == NULL) {
			/* ... */
			fprintf(stderr, "InitGameDirectories failure\n");
			exit(EXIT_FAILURE);
		}

		gamedir = strrchr(tmp, '/');

		if (gamedir) {
			*gamedir = 0;
			gamedir = tmp;
		
			if (!check_game_directory(gamedir)) {
				gamedir = NULL;
			}
		}
	}
	
	if (gamedir == NULL) {
		/* 3. realpath of executable path from argv[0] */
		
		assert(tmp != NULL);

#if defined(_MSC_VER)
		_fullpath( tmp, tmppath, PATH_MAX );
#else
		gamedir = realpath(tmp, tmppath);
#endif

		if (!check_game_directory(gamedir)) {
			gamedir = NULL;
		}
	}

	if (gamedir == NULL) {
		/* 4. $PATH */
		path = getenv("PATH");
		if (path) {
			while (*path) {
				len = strcspn(path, ":");
				
				copylen = min(len, PATH_MAX-1);
				
				strncpy(tmppath, path, copylen);
				tmppath[copylen] = 0;
				
				if (check_game_directory(tmppath)) {
					gamedir = tmppath;
					break;
				}
				
				path += len;
				path += strspn(path, ":");
			}
		}
	}
	
	if (gamedir == NULL) {
		/* 5. current directory */
		gamedir = ".";
	}
	
	assert(gamedir != NULL);
	
	/* last chance sanity check */
	if (!check_game_directory(gamedir)) {
		fprintf(stderr, "Unable to find the AvP gamedata.\n");
		fprintf(stderr, "The directory last examined was: %s\n", gamedir);
		fprintf(stderr, "Has the game been installed and\n");
		fprintf(stderr, "are all game files lowercase?\n");
		exit(EXIT_FAILURE);
	}

	SetGameDirectories(localdir, gamedir);
	
	free(localdir);
	if (tmp) {
		free(tmp);
	}
	
	/* delete some log files */
	DeleteGameFile("dx_error.log");
}

#ifdef FILES_DRIVER
int main(int argc, char *argv[])
{
	FILE *fp;
	char buf[64];
	void *dir;
	
	SetGameDirectories("tmp1", "tmp2");
	
	fp = OpenGameFile("tester", FILEMODE_WRITEONLY, FILETYPE_CONFIG);
	
	fputs("test\n", fp);
	
	CloseGameFile(fp);
	
	CreateGameDirectory("yaya");
	CreateGameDirectory("tester2");
	CreateGameDirectory("tester2/blah");
	
	fp = OpenGameFile("tester", FILEMODE_READONLY, FILETYPE_OPTIONAL);
	printf("Read: %s", fgets(buf, 60, fp));
	CloseGameFile(fp);
	
	fp = OpenGameFile("tester", FILEMODE_READONLY, FILETYPE_CONFIG);
	printf("Read: %s", fgets(buf, 60, fp));
	CloseGameFile(fp);
	
	dir = OpenGameDirectory(".", "*", FILETYPE_OPTIONAL);
	if (dir != NULL) {
		GameDirectoryFile *gd;
		
		while ((gd = ScanGameDirectory(dir)) != NULL) {
			printf("Name: %s, Attr: %08X\n", gd->filename, gd->attr);
		}
		
		CloseGameDirectory(dir); 
	} else {
		printf("Could not open the directory...\n");
	}
	
	DeleteGameFile("tester");
	 
	return 0;
}
#endif
