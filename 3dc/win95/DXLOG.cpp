#include "3dc.h"

#if debug

#include "ourasert.h"
#include "dxlog.h"
#include "debuglog.h"

/*
 * D3DAppErrorToString
 */
static char* D3DAppErrorToString(HRESULT error)
{
	return "D3DAppErrorToString function in Dxlog.c";
}

static const char *LOGFILE_NAME = "dx_error.log";

static LOGFILE *dxlog = 0;
static bool closed_once = false;

extern void exit_break_point_function();

void dx_err_log(HRESULT error, int line, char const * file)
{
	if (closed_once)
		return;
	
	if (!dxlog)
		dxlog = lfopen(LOGFILE_NAME);

//	lfprintf(dxlog,"Line %d of %s:\n%s\n\n",line,file,D3DAppErrorToString(error));
}

void dx_str_log(char const * str, int line, char const * file)
{
	if (closed_once)
		return;

	if (!dxlog)
		dxlog = lfopen(LOGFILE_NAME);

	lfprintf(dxlog, "Line %d of %s:\n%s\n\n", line, file, str);
}

void dx_line_log(int line, char const * file)
{
	if (closed_once)
		return;

	if (!dxlog)
		dxlog = lfopen(LOGFILE_NAME);

	lfprintf(dxlog, "Line %d of %s:\n", line, file);
}

void dx_strf_log(char const * fmt, ... )
{
	if (closed_once)
		return;

	if (!dxlog) dxlog = lfopen(LOGFILE_NAME);
	{
		va_list ap;
		va_start(ap, fmt);
		vlfprintf(dxlog,fmt,ap);
		va_end(ap);
	}
	lfputs(dxlog,"\n\n");
}

void dx_log_close()
{
	if (dxlog)
		lfclose(dxlog);
}

#undef exit
int GlobalAssertFired(char * Filename, int LineNum,char * Condition)
{
	exit_break_point_function();

	if (closed_once)
		return 0;

	if (!dxlog) 
		dxlog = lfopen(LOGFILE_NAME);

	lfprintf(dxlog, "Line %d of %s:\nGAF: %s\n", LineNum, Filename, Condition);
	lfclose(dxlog);
	closed_once = true;
	textprint("Line %d of %s:\nGAF: %s\n", LineNum, Filename, Condition);
	WaitForReturn();

	ExitSystem();
	exit(0xffff);
}

int LocalAssertFired(char * Filename, int LineNum,char * Condition)
{
	exit_break_point_function();

	if (closed_once)
		return 0;

	if (!dxlog)
		dxlog = lfopen(LOGFILE_NAME);

	lfprintf(dxlog, "Line %d of %s:\nLAF: %s\n", LineNum, Filename, Condition);
	lfclose(dxlog);
	closed_once = true;
	textprint("Line %d of %s:\nLAF: %s\n", LineNum, Filename, Condition);
	WaitForReturn();

	ExitSystem();
	exit(0xfffe);
}

void ExitFired(char* Filename, int LineNum, int ExitCode)
{
	exit_break_point_function();

	if (closed_once)
		return;

	if (!dxlog)
		dxlog = lfopen(LOGFILE_NAME);

	lfprintf(dxlog, "Line %d of %s:\nExit: %x\n", LineNum, Filename, ExitCode);
	lfclose(dxlog);
	closed_once = true;

	exit(ExitCode);
}

#endif
