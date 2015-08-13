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

#define FILETABLEHEADER_NAMESIZE (32)
#define FILETABLE_NAMESIZE (64)
#if WINDOWS
#define DIR_CHAR '\\'
#define DIR_STR "\\"
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
	char Name[FILETABLEHEADER_NAMESIZE];
};

/**
 * Intended to be formatted exactly like a FileTableEntry so they can be used interchangeably.
 * To determine if a FileTableEntry is really a Folder its FileSize will equal 0.
 */
struct Folder {
	char Filename[FILETABLEHEADER_NAMESIZE];
	Uint16 FileCt;
	Uint16 IsFile;
	Uint16 Formatting[6];
	struct FileTableEntry* Child;
	struct FileTableEntry* Next;
	struct Folder* Parent;
};

struct FileTableEntry {
	char Filename[FILETABLE_NAMESIZE];
	Uint16 FileSize;
	Uint16 Offset;
	struct FileTableEntry* Next;
	struct Folder* Parent;
};

struct Resource {
	int AlwaysCache;
	int* RefCt;
	int ResourceType;
	void* Data;
};

struct ResourceType {
	void*(*Create)(SDL_RWops*);
	void(*Destroy)(void*);
	int(*IsResource)(SDL_RWops*);
};

struct PakFile {
	struct ResourceType ResTypes[RESOURCETYPE_SIZE];
	struct FileTableHeader PakHeader;
	struct FileTableEntry* FileTable;
	struct Folder* CurrFolder;
	int PakFd;
};

struct ResourceManager {
	struct HashTable ResourceTable;
	struct PakFile* PakFiles[];
};

extern struct ResourceManager g_RsrMgr;

int ResourceManagementInit();
void ResourceManagementQuit();

char* GetFile(const char* _Filename, char* _Buffer, int* _Len);

void RsrMgrConstruct(struct PakFile* _Mgr, const char* _Filename);
void CreatePak(const char* _DirName);
int PakWriteData(int _Fd, const struct FileTableHeader* _Header, struct FileTableEntry* _FileTable, char* _Buffer);
int PakOpenFolder(struct PakFile* _Pak, const char* _Filename);
struct Resource* PakOpenFile(struct HashTable* _Table, struct PakFile* _Pak, const char* _Path);
void PakReadFile(struct PakFile* _Pak, const struct FileTableEntry* _FileTable, char* _Buffer);

struct FileTableEntry* CreateFileTableEntryBuff(const char* _Buffer);
struct FileTableEntry* CreateFileTableEntry(const char* _Name, Uint16 _FileSize);
void DestroyFileTable(struct FileTableEntry* _FileTable);

struct FileTableEntry* CreateFileTableChain(const char* _DirName, int* _Ct, struct Folder* _Parent);
struct FileTableEntry* RsrMgrLoadFile(struct PakFile* _Mgr, int _FileCt, char* _Buffer, struct Folder* _Parent);
struct FileTableEntry* NextFileEntry(struct FileTableEntry* _Table);
/**
 * Looks at the next file in the directory _Dir and creates a FileTableEntry for it.
 */
struct FileTableEntry* NextFile(DIR* _Dir);

struct Resource* CreateResource(const struct FileTableEntry* _Entry, const char* _Data);
void DestroyResource(struct Resource* _Resource);

#endif
