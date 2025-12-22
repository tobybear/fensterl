#include "../fenster.h"

#include <stdio.h>
#include <windows.h>

uint32_t w = 1024;
uint32_t h = 768;
uint32_t* buf;

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("No map file given.\n");
		return 1;
	}

	const char* mapfile = "/tmp/fstub";
	DWORD filesize = 200 + w * h * sizeof(uint32_t);

	void* hMapFile = (void*)CreateFileMappingA(
		INVALID_HANDLE_VALUE,	// use paging file
		NULL,					// default security
		PAGE_READWRITE,			// read/write access
		0,						// maximum object size (high-order DWORD)
		filesize,				// maximum object size (low-order DWORD)
		mapfile);               // name of mapping object

	if (hMapFile == NULL) {
		printf("Could not open file mapping object (%d).\n", GetLastError());
		return 1;
	}

	uint8_t* pBuf = (uint8_t*)MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0, NULL);

	if (pBuf == NULL) {
		printf("Could not map view of file (%d).\n", GetLastError());
		CloseHandle(hMapFile);
		return 1;
	}

	char title[100] = { 0 };
	memcpy(&title[0], &pBuf[0], strlen((const char *)pBuf));
	volatile uint8_t* sync = &pBuf[100];
	fenster_input_data* inp = (fenster_input_data*)&pBuf[101];
	uint32_t sz = sizeof(fenster_input_data);
	w = *(uint32_t*)(&pBuf[101 + sz]);
	h = *(uint32_t*)(&pBuf[106 + sz]);
	buf = (uint32_t*)(&pBuf[110 + sz]);

	struct fenster f = { 0 };
	f.title = title;
	f.width = w;
	f.height = h;
	f.buf = buf;
	fenster_open(&f);
	int64_t now = fenster_time();
	while (fenster_loop(&f) == 0) {
		if (*sync == 2) break;
		*inp = f.inp;

		*sync = 1;
		while (*sync == 1) { fenster_sleep(5); }
		int64_t time = fenster_time();
		if (time - now < 1000 / 60) {
			fenster_sleep(time - now);
		}
		now = time;
	}
	*sync = 2;
	fenster_close(&f);

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;
}

