/*
 * File: ResourceManager.c
 * Author: David Brotz
 */

#include "ResourceManager.h"

#include "Log.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <malloc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rwops.h>

#define PAK_SIG ("Pak02")

#define READ(_Buffer, _Off, _Size) 	\
	_Buffer[_Off];					\
	_Off += _Size

#define FILE_ERROR "Cannot open file: %s, %s"

#define ToUint(_DWord) ((Uint16)(((unsigned char)(_DWord)[3]) << 16)  | (((unsigned char)(_DWord)[2]) << 16)  | (((unsigned char)(_DWord)[1]) << 8) | (((unsigned char)(_DWord)[0])))

struct ResourceManager g_RsrMgr;
static struct ResourceType g_ResourceTypes[RESOURCE_SIZE] = {
		//{IMG_LoadPNG_RW, SDL_FreeTexture, IMG_isPNG}
};

int ResourceManagementInit(const char* _Files[RSRMGR_PAKSIZE]) {
	for(int i = 0; i < RSRMGR_PAKSIZE; ++i)
		RsrMgrConstruct(g_RsrMgr.PakFiles[i], _Files[i]);
	return 1;
}

void ResourceManagementQuit() {

}

char* GetFile(const char* _Filename, char* _Buffer, int* _Len) {
	int _File = 0;
#ifdef WINDOWS
	if((_File = open(_Filename, O_RDONLY | _O_BINARY)) < 0)
#else
	if((_File = open(_Filename, O_RDONLY)) < 0)
#endif
		return 0;
	int _Size = -1;

	_Size = lseek(_File, 0, SEEK_END);
	lseek(_File, 0, SEEK_SET);
	read(_File, _Buffer, _Size);
	close(_File);
	*_Len = _Size;
	return _Buffer;
}

int RsrMgrGenerateHFT(struct PakFile* _Mgr, const char* _Buffer) {
	int i = 0;
	int _SigLength = strlen(PAK_SIG) + 1;
	char _PakSig[_SigLength];

	_Mgr->PakHeader.Version = READ(_Buffer, i, sizeof(Uint16));
	for(int j = 0; j < _SigLength; i++, j++)
		_PakSig[j] = _Buffer[i];
	if(strcmp(_PakSig, PAK_SIG))
		return 0;
	_Mgr->PakHeader.FlTblCt = READ(_Buffer, i, sizeof(Uint16));
	for(int j = 0; j < FILETABLEHEADER_NAMESIZE; i++, j++)
		_Mgr->PakHeader.Name[j] = _Buffer[i];
	return 1;
}

/*
 * TODO: We know how many files there are before we call CreateFileTableEntry, we could just malloc them all at once.
 */
void RsrMgrConstruct(struct PakFile* _Mgr, const char* _Filename) {
	struct FileTableEntry* _Root = NULL;
	char _Buffer[sizeof(struct FileTableHeader) + 1];
	int _Error = 0;

	if((_Mgr->PakFd = open(_Filename, O_RDONLY | O_BINARY)) < 0) {
		Log(ELOG_WARNING, "Cannot open file: %s", _Filename);
		return;
	}
	if((_Error = read(_Mgr->PakFd, _Buffer, sizeof(struct FileTableHeader))) != sizeof(struct FileTableHeader)) {
		Log(ELOG_WARNING, "Cannot read file: %s. %s", _Filename, strerror(errno));
		return;
	}
	if(!RsrMgrGenerateHFT(_Mgr, _Buffer))
		return;
	if((_Root = RsrMgrLoadFile(_Mgr, _Mgr->PakHeader.FlTblCt, _Buffer, ((struct Folder*)_Root))) == NULL) {
		Log(ELOG_WARNING, "Cannot read pak %s. %s.", _Filename, strerror(errno));
		return;
	}
	_Mgr->FileTable = _Root;
	_Mgr->CurrFolder = (struct Folder*)_Root;
}

void CreatePak(const char* _DirName) {
	int _Ct = 0;
	int _Offset = 0;
	int _OutFile = 0;
	int _Error = 0;
	Uint16 _BufferLen = 0;
	int _PakSize = strlen(_DirName + 5);
	char _PakName[_PakSize];
	struct FileTableHeader _Header;
	struct FileTableEntry* _FTHeader = NULL;
	struct FileTableEntry* _FileTable = NULL;
	char* _Buffer = NULL;

	_PakName[0] = 0;
	_FTHeader = CreateFileTableChain(_DirName, &_Ct, NULL);
	/*
	 * Creating the FileTableHeader.
	 */
	_Header.Version = PAK_VERSION;
	_Header.FlTblCt = _Ct;
	strcat(_PakName, _DirName);
	strcat(_PakName, ".pak");
	strcpy(_Header.Name, _PakName);

	if(_FTHeader == NULL)
		goto end;
	/*
	 * Writing the data.
	 */
	if((_OutFile = open(_Header.Name, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, S_IREAD | S_IWRITE)) < 0)
		goto end;
	if((_Error = write(_OutFile, (char*)&_Header, sizeof(struct FileTableHeader))) != sizeof(struct FileTableHeader)) {
		Log(ELOG_ERROR, FILE_ERROR, _Header.Name, strerror(errno));
		goto end;
	}
	_Offset = sizeof(struct FileTableHeader) + (sizeof(struct FileTableEntry) * _Ct);
	_FileTable = _FTHeader;
	do {
		if(((struct Folder*)_FileTable)->IsFile > 0) {
			_FileTable->Offset = _Offset;
			_Offset += _FileTable->FileSize;
			if(_FileTable->FileSize > _BufferLen) {
				_BufferLen = _FileTable->FileSize;
			}
		}
		if((_Error = write(_OutFile, _FileTable, sizeof(struct FileTableEntry))) != sizeof(struct FileTableEntry)) {
			Log(ELOG_ERROR, FILE_ERROR, _Header.Name, strerror(errno));
			goto end;
		}
	} while((_FileTable = NextFileEntry(_FileTable)) != NULL);
	_Buffer = (char*) alloca(sizeof(char) * (_BufferLen + 1));
	_FileTable = _FTHeader;
	chdir(_DirName);
		if(PakWriteData(_OutFile, &_Header, _FileTable, _Buffer) == 0)
			goto end;
	chdir("..");
	end:
	if(_FTHeader)
		DestroyFileTable(_FTHeader);
	close(_OutFile);
}

int PakWriteData(int _Fd, const struct FileTableHeader* _Header, struct FileTableEntry* _FileTable, char* _Buffer) {
	int _FileLen = 0;

	if(_FileTable == NULL)
		return 0;
	do {
		if(((struct Folder*)_FileTable)->IsFile == 0) {
			chdir(_FileTable->Filename);
			PakWriteData(_Fd, _Header, ((struct Folder*)_FileTable)->Child, _Buffer);
			chdir("..");
			continue;
		}
		GetFile(_FileTable->Filename, _Buffer, &_FileLen);
		if(write(_Fd, _Buffer, _FileLen) != _FileLen)
			return 0;
	} while((_FileTable = _FileTable->Next) != NULL);
	return 1;
}

int PakOpenFolder(struct PakFile* _Pak, const char* _Filename) {
	char _Str[strlen(_Filename) + 1];
	char* _Ptr = NULL;
	struct FileTableEntry* _Table = 0;

	if(_Pak->CurrFolder == 0)
		return 0;
	_Table = (struct FileTableEntry*)_Pak->CurrFolder;

	strcpy(_Str, _Filename);
	_Ptr = strtok(_Str, "/\\");
	while(_Ptr != NULL) {
		if(!strcmp(_Ptr, _Table->Filename)) {
			if(((struct Folder*)_Table)->IsFile == 0) {
				_Pak->CurrFolder = (struct Folder*)_Table;
				_Table = ((struct Folder*)_Table)->Child;
				goto file_found;
			} else {
				_Pak->CurrFolder = (struct Folder*)_Table->Parent;
				return 1;
			}
		} else
			_Table = _Table->Next;
		if(_Table == NULL) {
			return 0;
		}
		file_found:
		_Ptr = strtok(NULL, "/\\");
	}
	_Pak->CurrFolder = (((struct Folder*)_Table)->IsFile == 0) ? ((struct Folder*)_Table) : (struct Folder*)_Table->Parent;
	return 1;
}

struct Resource* PakOpenFile(struct HashTable* _Table, struct PakFile* _Pak, const char* _Path) {
	char _Str[strlen(_Path) + 1];
	char* _Ptr = NULL;
	char* _Filename;
	struct FileTableEntry* _Entry = NULL;
	struct Resource* _File = NULL;
	int _Found = 0;
	struct Folder* _OrigDir = _Pak->CurrFolder;

	if(_Path == NULL)
		return NULL;

	strcpy(_Str, _Path);
	_Filename = strchr(_Path, '\\');
	if(_Filename == NULL)
		_Filename = _Str;
	else
		++_Filename;
	_Ptr = strtok(_Str, "/\\");
	while(PakOpenFolder(_Pak, _Ptr) != 0 && (_Ptr = strtok(NULL, "/\\")) != NULL);
	_Entry = (struct FileTableEntry*)_Pak->CurrFolder->Child;
	while(_Entry != NULL) {
		if(!strcmp(_Filename, _Entry->Filename)) {
			_Found = ((_File = HashSearch(_Table, _Path)) != 0);
			if(_Found != 0) {
				char* _Buffer = (char*) calloc(_Entry->FileSize + 1, sizeof(char)); //Done to not stack overflow with a several megabyte file.

				PakReadFile(_Pak, _Entry, _Buffer);
				_File = CreateResource(_Entry, _Buffer);
				free(_Buffer);
			}
			_Pak->CurrFolder  = _OrigDir;
			return _File;
		}
		_Entry = _Entry->Next;
	}
	_Pak->CurrFolder = _OrigDir;
	return NULL;
}

void PakReadFile(struct PakFile* _Pak, const struct FileTableEntry* _FileTable, char* _Buffer) {

	lseek(_Pak->PakFd, _FileTable->Offset, SEEK_SET);
	if(read(_Pak->PakFd, _Buffer, _FileTable->FileSize) == -1)
		Log(ELOG_WARNING, strerror(errno));
	_Buffer[_FileTable->FileSize] = 0;
}

struct Resource* RsrMgrOpenFile(struct ResourceManager* _Mgr, int _Pak, const char* _File) {
	if(_Pak >= RSRMGR_PAKSIZE || _Mgr->PakFiles[_Pak] == NULL)
		return NULL;
	return PakOpenFile(&_Mgr->ResourceTable, _Mgr->PakFiles[_Pak], _File);
}

struct FileTableEntry* CreateFileTableEntryBuff(const char* _Buffer) {
	int _Index = 0;
	struct FileTableEntry* _Entry = (struct FileTableEntry*) malloc(sizeof(struct FileTableEntry));

	for(int i = 0; i < FILETABLE_NAMESIZE; i++) {
		_Entry->Filename[i] = READ(_Buffer, _Index, sizeof(char));
	}
	_Entry->FileSize = ToUint(_Buffer + _Index);
	_Index += sizeof(Uint16);
	_Entry->Offset = ToUint(_Buffer + _Index);
	_Index += sizeof(Uint16);
	return _Entry;
}

struct FileTableEntry* CreateFileTableEntry(const char* _Name, Uint16 _FileSize) {
	struct FileTableEntry* _FileTable = (struct FileTableEntry*) malloc(sizeof(struct FileTableEntry));

	for(int i = 0; i < FILETABLE_NAMESIZE; i++)
		_FileTable->Filename[i] = _Name[i];
	if(((struct Folder*)_FileTable)->IsFile == 0)
		((struct Folder*)_FileTable)->IsFile = 1;
	_FileTable->FileSize = _FileSize;
	_FileTable->Offset = 0;
	_FileTable->Next = NULL;
	_FileTable->Parent = NULL;
	return _FileTable;
}

void DestroyFileTable(struct FileTableEntry* _FileTable) {
	struct FileTableEntry* _Entry = _FileTable;
	struct FileTableEntry* _Next = NULL;

	if(_Entry != NULL)
		_Next = NextFileEntry(_Entry);
	else
		return;
	while(_Next != NULL) {
		if(((struct Folder*)_Entry)->IsFile == 0)
			DestroyFileTable(((struct Folder*)_Entry)->Child);
		free(_Entry);
		_Entry = _Next;
		_Next = _Entry->Next;
	}
	if(((struct Folder*)_Entry)->IsFile == 0)
		DestroyFileTable(((struct Folder*)_Entry)->Child);
	free(_Entry);
}

struct FileTableEntry* CreateFileTableChain(const char* _DirName, int* _Ct, struct Folder* _Parent) {
	struct FileTableEntry* _FTHeader = NULL;
	struct FileTableEntry* _LastTable = NULL;
	struct FileTableEntry* _FileTable = NULL;
	DIR* _Dir = opendir(_DirName);

	if(_Dir == NULL) {
		Log(ELOG_ERROR, FILE_ERROR, _DirName, strerror(errno));
		return NULL;
	}
	chdir(_DirName);
	do {
		while(_LastTable == NULL) {
			_LastTable = NextFile(_Dir);
			if(errno == EBADF)
				break;
		}
		_FTHeader = _LastTable;
		do {
			_FileTable = NextFile(_Dir);
			if(((struct Folder*)_LastTable)->IsFile == 0) {
				int _CtTemp = *_Ct;
				int _Size = strlen(_DirName) + strlen(_LastTable->Filename) + 2;
				char _NewDir[_Size];
				//InitString(_NewDir, _Size);
				strcat(_NewDir, _LastTable->Filename);
				//strcat(_NewDir, DIR_STR);
				((struct Folder*)_LastTable)->Child = CreateFileTableChain(_NewDir, _Ct, (struct Folder*)_LastTable);
				((struct Folder*)_LastTable)->FileCt = (*_Ct) - _CtTemp;
			}
			_LastTable->Parent = _Parent;
			_LastTable->Next = _FileTable;
			_LastTable = _FileTable;
			++(*_Ct);
		} while(_FileTable != 0);
	} while(0);
	chdir("..");
	closedir(_Dir);
	_Dir = 0;
	return _FTHeader;
}

struct FileTableEntry* RsrMgrLoadFile(struct PakFile* _Mgr, int _FileCt, char* _Buffer, struct Folder* _Parent) {
	struct FileTableEntry* _Entry = NULL;
	struct FileTableEntry* _Last = NULL;
	struct FileTableEntry* _First = NULL;

	if(read(_Mgr->PakFd, _Buffer, sizeof(struct FileTableEntry)) != sizeof(struct FileTableEntry))
		return NULL;
	_First = CreateFileTableEntryBuff(_Buffer);
	_First->Parent = _Parent;
	_Last = _First;
	--_FileCt;

	while(1) {
		if(((struct Folder*)_Last)->IsFile == 0) {
			((struct Folder*)_Last)->Child = RsrMgrLoadFile(_Mgr, ((struct Folder*)_Last)->FileCt, _Buffer, (struct Folder*)_Last);
			_FileCt -= ((struct Folder*)_Last)->FileCt;
		}

		if(_FileCt <= 0)
			break;
		if(read(_Mgr->PakFd, _Buffer, sizeof(struct FileTableEntry)) == 0) {
			return NULL;
		}
		_Entry = CreateFileTableEntryBuff(_Buffer);
		_Entry->Parent = _Parent;
		_Last->Next = _Entry;
		_Last = _Entry;
		--_FileCt;
	};
	_Last->Next = 0;
	return _First;
}

struct FileTableEntry* NextFileEntry(struct FileTableEntry* _Table) {
	if(_Table == NULL)
		return NULL;
	if(((struct Folder*)_Table)->IsFile == 0) {
		struct FileTableEntry* _Parent = _Table;
		_Table = ((struct Folder*)_Table)->Child;
		if(_Table == NULL)
			return _Parent->Next;
		return _Table;
	}
	if(_Table->Next != NULL)
		return _Table->Next;
	else if(_Table->Parent != NULL)
		return _Table->Parent->Next;
	return NULL;
}

struct FileTableEntry* NextFile(DIR* _Dir) {
	struct stat _Stat;
	struct dirent* _Dirent = readdir(_Dir);

	errno = 0;
	if(_Dirent != NULL) {
		if(!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, ".."))
				return NULL;
		if(stat(_Dirent->d_name, &_Stat) == -1) {
			Log(ELOG_ERROR, "Bad file %s", _Dirent->d_name);
			goto fail;
		}

		if(S_ISDIR(_Stat.st_mode)) {
			struct Folder* _Folder = (struct Folder*) malloc(sizeof(struct Folder));
			for(int i = 0; i < FILETABLE_NAMESIZE; i++)
				_Folder->Filename[i] = _Dirent->d_name[i];
			_Folder->FileCt = 0;
			_Folder->IsFile = 0;
			_Folder->Next = NULL;
			return (struct FileTableEntry*)_Folder;
		}
		return CreateFileTableEntry(_Dirent->d_name, _Stat.st_size);
	}
	return NULL;
	fail:
	errno = EBADF;
	return NULL;
}

struct Resource* CreateResource(const struct FileTableEntry* _Entry, const char* _Data) {
	struct Resource* _Resource = (struct Resource*) malloc(sizeof(struct Resource));

	_Resource->RefCt = (int*) malloc(sizeof(int));
	(*_Resource->RefCt) = 1;
	_Resource->AlwaysCache = 1;
	for(int i = 0; i < RESOURCE_SIZE; ++i) {
		SDL_RWops* _Ops = SDL_RWFromConstMem(_Data, _Entry->FileSize);

		if(g_ResourceTypes[i].IsResource(_Ops) && (_Resource->Data = g_ResourceTypes[i].Create(_Ops)) != NULL) {
			_Resource->ResourceType = i;
			SDL_RWclose(_Ops);
			break;
		}
		SDL_RWclose(_Ops);
	}
	return _Resource;
}

void DestroyResource(struct Resource* _Resource) {
	--(*_Resource->RefCt);
	if((*_Resource->RefCt) <= 0)
		free(_Resource);
}
