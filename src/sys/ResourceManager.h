/*
 * File: ResourceManager.h
 * Author: David Brotz
 */
#ifndef __RESOURCEMANAGER_H
#define __RESOURCEMANAGER_H

#include <SDL2/SDL_stdinc.h>

#include <dirent.h>

struct HashTable;

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
	int RefCt;
	int ResourceType;
	void* Data;
};

struct ResourceType {
	void*(*CreateResource)(const char*);
	int(*IsResourceType)(const char*);
};

struct ResourceManager {
	struct HashTable* ResourceTable;
	struct ResourceType ResTypes[RESOURCETYPE_SIZE];
	struct FileTableHeader PakHeader;
	struct FileTableEntry* FileTable;
	struct Folder* CurrFolder;
	int PakFd;
};

char* GetFile(const char* _Filename, char* _Buffer, int* _Len);

void RsrMgrConstruct(struct ResourceManager* _Mgr, const char* _Filename);
void RsrMgrCreatePak(const char* _DirName);
int ResMgrWriteData(int _Fd, const struct FileTableHeader* _Header, struct FileTableEntry* _FileTable, char* _Buffer);

struct FileTableEntry* CreateFileTableEntryBuff(const char* _Buffer);
struct FileTableEntry* CreateFileTableEntry(const char* _Name, Uint16 _FileSize);
void DestroyFileTable(struct FileTableEntry* _FileTable);

struct FileTableEntry* CreateFileTableChain(const char* _DirName, int* _Ct, struct Folder* _Parent);
struct FileTableEntry* RsrMgrLoadFile(struct ResourceManager* _Mgr, int _FileCt, char* _Buffer, struct Folder* _Parent);
struct FileTableEntry* NextFileEntry(struct FileTableEntry* _Table);
struct FileTableEntry* ResMgrNextFile(DIR* _Dir);

#endif
