/*
 * File: ResourceManager.h
 * Author: David Brotz
 */
#ifndef __RESOURCEMANAGER_H
#define __RESOURCEMANAGER_H

#include "HashTable.h"

#include <SDL2/SDL_stdinc.h>

#include <dirent.h>

#define PAK_VERSION (5)

struct HashTable;
typedef struct SDL_RWops SDL_RWops;
struct FileTableEntry;
struct Folder;
struct Resource;

#define FILETABLEHEADER_NAMESIZE (60)
#define FILETABLE_NAMESIZE (64)
#define FILETABLE_SIGSZ (6)
#if WINDOWS
#define DIR_CHAR '\\'
#define DIR_STR "\\"
#endif

#ifndef WINDOWS
	#define O_BINARY 0
	#define S_IREAD S_IRUSR
	#define S_IWRITE S_IWUSR
#endif

#if LINUX
#define DIR_CHAR '/'
#define DIR_STR "/"
#endif

enum {
	RESOURCETYPE_GRAPHIC,
	RESOURCETYPE_SIZE
};

enum {
	RSRMGR_GRAPHICS,
	RSRMGR_PAKSIZE
};

enum {
	RESOURCE_PNG,
	RESOURCE_SIZE
};

struct FileTableHeader {
	Uint16 Version;
	Uint16 FlTblCt;
	char Sig[FILETABLE_SIGSZ];
	char Name[FILETABLEHEADER_NAMESIZE];
};

struct ResourceType {
	void*(*Create)(SDL_RWops*);
	void(*Destroy)(void*);
	int(*IsResource)(SDL_RWops*);
};

struct PakFile {
	struct FileTableHeader PakHeader;
	struct FileTableEntry* FileTable;
	struct Folder* CurrFolder;
	int PakFd;
};

struct ResourceManager {
	struct HashTable ResourceTable;
	struct ResourceType* ResTypes;
};

extern struct ResourceManager g_RsrMgr;

int ResourceManagementInit(int _FileTypes);
void ResourceManagementQuit();
/*
 * TODO Remove function declarations for private functions.
 */

//char* GetFile(const char* _Filename, char* _Buffer, int* _Len);

void RsrMgrConstruct(struct PakFile* _Mgr, const char* _Filename);
void CreatePak(const char* _DirName);
void DestroyPak(struct PakFile* _Pak);
int PakOpenFolder(struct PakFile* _Pak, const char* _Filename);
//struct Resource* PakOpenFile(struct HashTable* _Table, struct PakFile* _Pak, const char* _Path);
//void PakReadFile(const struct PakFile* _Pak, const struct FileTableEntry* _FileTable, char* _Buffer);

/*struct FileTableEntry* CreateFileTableEntryBuff(const char* _Buffer);
struct FileTableEntry* CreateFileTableEntry(const char* _Name, Uint16 _FileSize);
void DestroyFileTable(struct FileTableEntry* _FileTable);

struct FileTableEntry* CreateFileTableChain(const char* _DirName, int* _Ct, struct Folder* _Parent);
struct FileTableEntry* RsrMgrLoadFile(struct PakFile* _Mgr, int _FileCt, char* _Buffer, struct Folder* _Parent);
struct FileTableEntry* NextFileEntry(struct FileTableEntry* _Table);
struct FileTableEntry* NextFile(DIR* _Dir);*/

//struct Resource* CreateResource(const struct PakFile* _Pak, const struct FileTableEntry* _Entry);
void DestroyResource(struct Resource* _Resource);
struct Resource* ResourceGet(const char* _FilePath);
int ResourceExists(const char* _FilePath);
void* ResourceGetData(const struct Resource* _Res);

#endif
