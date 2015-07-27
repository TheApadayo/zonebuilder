#include "StdInc.h"
#include <shellapi.h>

extern bool console;
void Sys_Print(const char* message);

CRITICAL_SECTION logfile_cs;
FILE* logfile;

void Com_Logging_Init(const char* file)
{
#if ZB_DEBUG
	InitializeCriticalSection(&logfile_cs);
	logfile = fopen(file, "w");
#endif
}

void Com_Logging_Quit()
{
#if ZB_DEBUG
	EnterCriticalSection(&logfile_cs);
	fclose(logfile);
	LeaveCriticalSection(&logfile_cs);
	DeleteCriticalSection(&logfile_cs);
#endif
}

void Com_Quit()
{
	Com_Logging_Quit();
	TerminateProcess(GetCurrentProcess(), -1);
}

void Com_Printf_(bool logOnly, const char* format, ...)
{
	static char buffer[32768] = { 0 };

	va_list va;
	va_start(va, format);

	//vprintf(format, va);
	_vsnprintf(buffer, sizeof(buffer), format, va);
	va_end(va);

	if (!logOnly)
	{
		if (console)
			Sys_Print(buffer);
		else
			printf(buffer);
	}

#if ZB_DEBUG
	EnterCriticalSection(&logfile_cs);
	fprintf(logfile, buffer);
	fflush(logfile);
	LeaveCriticalSection(&logfile_cs);
#endif
}

void Com_Debug_(bool logOnly, const char* format, ...)
{
	static char buffer[32768] = { 0 };

	va_list va;
	va_start(va, format);

	//vprintf(format, va);
	_vsnprintf(buffer, sizeof(buffer), format, va);
	va_end(va);
	if (!logOnly)
	{
		if (console){
			Sys_Print("^3");
			Sys_Print(buffer);
			Sys_Print("^7");
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
			printf(buffer);
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		}
	}

#if ZB_DEBUG
	EnterCriticalSection(&logfile_cs);
	fprintf(logfile, buffer);
	fflush(logfile);
	LeaveCriticalSection(&logfile_cs);
#endif
}

void Com_Error(bool exit, const char* format, ...)
{
	static char buffer[32768] = { 0 };

	va_list va;
	va_start(va, format);

	//vprintf(format, va);
	_vsnprintf(buffer, sizeof(buffer), format, va);
	va_end(va);

	if (console){
		Sys_Print("^1ERROR: ");
		Sys_Print(buffer);
		Sys_Print("^7\n");
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		printf("ERROR: %s\n", buffer);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	}

#if ZB_DEBUG
	EnterCriticalSection(&logfile_cs);
	fprintf(logfile, "ERROR: %s\n", buffer);
	fflush(logfile);
	LeaveCriticalSection(&logfile_cs);
#endif

	if (exit)
	{
		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
		Com_Quit();
	}
}

bool waitingOnLoad = false;
const char* zoneWaitingOn;

void Com_LoadZones(XZoneInfo* zones, int count)
{
	zoneWaitingOn = zones[count - 1].name;
	waitingOnLoad = true;

	DB_LoadXAssets(zones, count, 0);

	while (waitingOnLoad) Sleep(100);
}

void Com_UnloadZones(int group)
{
	XZoneInfo unload;
	unload.name = NULL;
	unload.type1 = 0;
	unload.type2 = group;
	DB_LoadXAssets(&unload, 1, 0);
}

const char* assetTypeStrings [] = {
	"physpreset",
	"phys_collmap",
	"xanim",
	"xmodelsurfs",
	"xmodel",
	"material",
	"pixelshader",
	"vertexshader",
	"vertexdecl",
	"techset",
	"image",
	"sound",
	"sndcurve",
	"loaded_sound",
	"col_map_sp",
	"col_map_mp",
	"com_map",
	"game_map_sp",
	"game_map_mp",
	"map_ents",
	"fx_map",
	"gfx_map",
	"lightdef",
	"ui_map",
	"font",
	"menufile",
	"menu",
	"localize",
	"weapon",
	"snddriverglobals",
	"fx",
	"impactfx",
	"aitype",
	"mptype",
	"character",
	"xmodelalias",
	"rawfile",
	"stringtable",
	"leaderboarddef",
	"structureddatadef",
	"tracer",
	"vehicle",
	"addon_map_ents",
};

int getAssetTypeForString(const char* str)
{
	int i = 42;

	while(i > 0)
	{
		if(!strcmp(assetTypeStrings[i], str)) return i;
		i--;
	}

	return -1;
}

const char* getAssetStringForType(int type)
{
	return assetTypeStrings[type];
}

const char* getAssetName(int type, void* data)
{
	if (type == ASSET_TYPE_LOCALIZE) return ((Localize*)data)->name;
	if (type == ASSET_TYPE_IMAGE) return ((GfxImage*)data)->name;
	return ((Rawfile*)data)->name;
}

void setAssetName(int type, void* data, const char* name)
{
	if (type == ASSET_TYPE_LOCALIZE) ((Localize*)data)->name = strdup(name);
	else if (type == ASSET_TYPE_IMAGE) ((GfxImage*)data)->name = strdup(name);
	else ((Rawfile*)data)->name = strdup(name);
}

int iArgCount = 0;

int getArgc()
{
	return iArgCount;
}

LPSTR* getArgs()
{
	int iIdx;

	LPWSTR * pszArgVectorWide = CommandLineToArgvW(GetCommandLineW(), &iArgCount);

	// Convert the wide string array into an ANSI array (input is ASCII-7)
	LPSTR * pszArgVectorAnsi = new LPSTR [iArgCount];

	for (iIdx = 0; iIdx < iArgCount; ++iIdx)
	{
		 size_t qStrLen = wcslen(pszArgVectorWide[iIdx]), qConverted = 0;
		 pszArgVectorAnsi[iIdx] = new CHAR [qStrLen+1];
		 wcstombs_s(&qConverted,pszArgVectorAnsi[iIdx],qStrLen+1,
		 pszArgVectorWide [iIdx],qStrLen+1);
	}

	return pszArgVectorAnsi;
}

long flength(FILE* fp)
{
	long i = ftell(fp);
	fseek(fp, 0, SEEK_END);
	long ret = ftell(fp);
	fseek(fp, i, SEEK_SET);
	return ret;
}

unsigned int alignTo(unsigned int value, unsigned int alignment)
{
	return (~alignment & (alignment + value));
}

extern unsigned int R_HashString(const char* string);
void debugChecks()
{
	ASSERT(sizeof(PhysGeomList) == 0x48);
	ASSERT(sizeof(PhysGeomInfo) == 0x44);
	ASSERT(sizeof(BrushWrapper) == 0x44);
	ASSERT(sizeof(cPlane) == 0x14);
	ASSERT(sizeof(cBrushSide) == 8);
	ASSERT(sizeof(XAnim) == DB_GetXAssetTypeSize(ASSET_TYPE_XANIM));
	ASSERT(sizeof(XModelSurfaces) == DB_GetXAssetTypeSize(ASSET_TYPE_XMODELSURFS));
	ASSERT(sizeof(XModel) == DB_GetXAssetTypeSize(ASSET_TYPE_XMODEL));
	ASSERT(sizeof(Material) == 0x60);
	ASSERT(sizeof(GfxImage) == 0x20);
	ASSERT(sizeof(SoundAliasList) == DB_GetXAssetTypeSize(ASSET_TYPE_SOUND));
	ASSERT(sizeof(SoundAlias) == 100);
	ASSERT(sizeof(SpeakerMap) == 408);
	ASSERT(sizeof(SoundFile) == 12);
	ASSERT(sizeof(WeaponVariantDef) == 0x74);
	ASSERT(sizeof(WeaponDef) == 0x684)
}