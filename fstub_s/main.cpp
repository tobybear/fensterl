#include "../fenster.h"

#include <stdio.h>
#include <windows.h>

uint32_t w = 1024;
uint32_t h = 768;
uint32_t* buf;

#define set_pixel(x, y) (buf[((y) * w) + (x)])

static void fenster_rect(int x, int y, int width, int height, uint32_t c) {
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			set_pixel(x + col, y + row) = c;
		}
	}
}

static uint16_t font5x3[] = {0x0000,0x2092,0x002d,0x5f7d,0x279e,0x52a5,0x7ad6,0x0012,0x4494,0x1491,0x017a,0x05d0,0x1400,0x01c0,0x0400,0x12a4,0x2b6a,0x749a,0x752a,0x38a3,0x4f4a,0x38cf,0x3bce,0x12a7,0x3aae,0x49ae,0x0410,0x1410,0x4454,0x0e38,0x1511,0x10e3,0x73ee,0x5f7a,0x3beb,0x624e,0x3b6b,0x73cf,0x13cf,0x6b4e,0x5bed,0x7497,0x2b27,0x5add,0x7249,0x5b7d,0x5b6b,0x3b6e,0x12eb,0x4f6b,0x5aeb,0x388e,0x2497,0x6b6d,0x256d,0x5f6d,0x5aad,0x24ad,0x72a7,0x6496,0x4889,0x3493,0x002a,0xf000,0x0011,0x6b98,0x3b79,0x7270,0x7b74,0x6750,0x95d6,0xb9ee,0x5b59,0x6410,0xb482,0x56e8,0x6492,0x5be8,0x5b58,0x3b70,0x976a,0xcd6a,0x1370,0x38f0,0x64ba,0x3b68,0x2568,0x5f68,0x54a8,0xb9ad,0x73b8,0x64d6,0x2492,0x3593,0x03e0};
static void fenster_text(int x, int y, char *s, int scale, uint32_t c) {
	while (*s) {
		char chr = *s++;
		if (chr > 32) {
			uint16_t bmp = font5x3[chr - 32];
			for (int dy = 0; dy < 5; dy++) {
				for (int dx = 0; dx < 3; dx++) {
					if (bmp >> (dy * 3 + dx) & 1) {
						fenster_rect(x + dx * scale, y + dy * scale, scale, scale, c);
					}
				}
			}
		}
		x = x + 4 * scale;
	}
}

int main(int argc, char** argv) {
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

	uint8_t* pBuf = (uint8_t*)MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, filesize, NULL);
	if (pBuf == NULL) {
		printf("Could not map view of file (%d).\n", GetLastError());
		CloseHandle(hMapFile);
		return 1;
	}

	memset(pBuf, 0, 120);
	const char* title = "This is a test";
	memcpy(pBuf, title, strlen(title));
	volatile uint8_t* sync = &pBuf[100];
	fenster_input_data* inp = (fenster_input_data*)&pBuf[101];
	uint32_t sz = sizeof(fenster_input_data);
	*(uint32_t*)(&pBuf[101 + sz]) = w;
	*(uint32_t*)(&pBuf[106 + sz]) = h;
	buf = (uint32_t*)(&pBuf[110 + sz]);

	do {
		while (*sync == 0) { fenster_sleep(5); }
		if (*sync == 2) break;

		int has_keys = 0;
		char s[32];
		char *p = s;
		for (int i = 32; i < 128; i++) {
			if (inp->keys[i]) {
				has_keys = 1;
				*p++ = i;
			}
		}
		*p = '\0';
		fenster_rect(0, 0, w, h, 0);
		int sc = 4;
		if (inp->x > 5 && inp->y > 5 && inp->x < w - 10 && inp->y < h - 10) {
			fenster_rect(inp->x - 3, inp->y - 3, 6, 6, inp->mouse ? 0xffffff : 0xff0000);
		}
		fenster_text(8, 8, s, sc, 0xffffff);
		if (has_keys) {
			if (inp->mod & 1) {
				fenster_text(8, 40, "Ctrl", sc, 0xffffff);
			}
			if (inp->mod & 2) {
				fenster_text(8, 80, "Shift", sc, 0xffffff);
			}
			if (inp->mod & 4) {
				fenster_text(8, 120, "Alt", sc, 0xffffff);
			}
			if (inp->mod & 8) {
				fenster_text(8, 160, "OS", sc, 0xffffff);
			}
		}

		if (inp->keys[27]) { break; }		

		*sync = 0;
	} while (true);
	*sync = 2;

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;
}

