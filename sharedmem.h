#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __COSMOCC__
#include "include/cosmo.h"
#include "include/windowsesque.h"
#include "include/libc/dce.h"
#include "include/libc/nt/runtime.h"
#include "include/libc/nt/memory.h"
#include "include/libc/nt/enum/pageflags.h"
#include "include/libc/sysv/consts/o.h"  
#include "include/libc/sysv/consts/prot.h" 
#include "include/libc/sysv/consts/map.h" 
#define INVALID_HANDLE_VALUE kNtInvalidHandleValue
#define PAGE_READWRITE kNtPageReadwrite
#define FILE_MAP_ALL_ACCESS 983071
#define HDL_TYPE int64_t
#else
#ifdef _WIN32
#include <windows.h>
#define HDL_TYPE HANDLE
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif
#endif

#if defined(_WIN32) || defined(__COSMOCC__)
void* create_sharedmem_win(const char* name, const uint32_t mem_size, const uint8_t reset, void** handle) {

	int i;
	int j = 0;
	uint8_t* p_buf;
	size_t name_len = strlen(name);
	unsigned char* name_w = (unsigned char*)malloc(name_len * 2 + 2);
	memset(name_w, 0, name_len * 2 + 2);
	for (i = 0; i < name_len; i++) {
		name_w[j] = (unsigned char)name[i];
		j += 2;
	}

	*handle = (void*)CreateFileMapping(
		INVALID_HANDLE_VALUE,	// use paging file
		NULL,					// default security
		PAGE_READWRITE,			// read/write access
		0,						// maximum object size (high-order DWORD)
#ifdef __COSMOCC__
		mem_size,			    // maximum object size (low-order DWORD)
		(const char16_t*)name_w);    // name of mapping object
#else
		(DWORD)mem_size,		// maximum object size (low-order DWORD)
		(LPCWSTR)name_w);       // name of mapping object
#endif

	if (*handle == NULL) {
		printf("Could not open file mapping object (%s, %d).\n", name, GetLastError());
		return NULL;
	}
	p_buf = (uint8_t*)MapViewOfFileEx((HDL_TYPE)*handle, FILE_MAP_ALL_ACCESS, 0, 0, (size_t)mem_size, NULL);
	if (p_buf == NULL) {
		printf("Could not map view of file (%s, %d).\n", name, GetLastError());
		CloseHandle((HDL_TYPE)*handle);
		return NULL;
	}
	if (reset > 0) memset(p_buf, 0, mem_size);
	free(name_w);
	return (void*)p_buf;
}

void destroy_sharedmem_win(void* pMem, void** handle) {
	UnmapViewOfFile(pMem);
	if (handle) {
		CloseHandle((HDL_TYPE)*handle);
	}
}

#endif

#if !defined(_WIN32) || defined(__COSMOCC__)
void* create_sharedmem_mmap(const char* name, const uint32_t size, const uint8_t reset, void** handle) {
	*handle = NULL;
	int fd = open(name, O_RDWR | O_CREAT | (reset > 0 ? O_TRUNC : 0 ), (mode_t)0600);
	if (fd == -1) {
		printf("Error opening file %s.\n", name);
		return NULL;
	}
	*handle = (void*)(uintptr_t)size;
	int result = lseek(fd, size - 1, SEEK_SET);
	if (result == -1) {
		close(fd);
		printf("Error calling lseek.\n");
		return NULL;
	}
	if (reset > 0) result = write(fd, "", 1);

	void* pMem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (pMem == MAP_FAILED) {
		close(fd);
		printf("Error mmapping file.\n");
		return NULL;
	}
	close(fd);
	return pMem;
}

void destroy_sharedmem_mmap(void* pMem, void** handle) {
	munmap(pMem, (uint32_t)(uintptr_t)(*handle));
}
#endif

void* create_sharedmem(const char* name, const uint32_t size, const uint8_t reset, void** handle) {
#ifdef __COSMOCC__
if (IsWindows()) {
	return create_sharedmem_win(name, size, reset, handle);
} else {
	return create_sharedmem_mmap(name, size, reset, handle);
}
#else
#ifdef _WIN32
return create_sharedmem_win(name, size, reset, handle);
#else
return create_sharedmem_mmap(name, size, reset, handle);
#endif
#endif
}

void destroy_sharedmem(void* pMem, void** handle) {
#ifdef __COSMOCC__
if (IsWindows()) {
	destroy_sharedmem_win(pMem, handle);
} else {
	destroy_sharedmem_mmap(pMem, handle);
}
#else
#ifdef _WIN32
destroy_sharedmem_win(pMem, handle);
#else
destroy_sharedmem_mmap(pMem, handle);
#endif
#endif
}
