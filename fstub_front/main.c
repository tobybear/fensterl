#define FENSTER_IMPLEMENTATION
#include "../fenster.h"
#include "../fenster_sync.h"
#include "../sharedmem.h"

#include <stdio.h>

#ifndef _WIN32
#define strcpy_s(a, b, c) strcpy((a), (c));
#endif

int main(int argc, char** argv) {
	struct fenster f = { 0 };
	int64_t now, time;
	const char* mapfile1 = "/tmp/fstub";
	const char* mapfile2 = "/tmp/fstub_pix";
	uint8_t* p_mem;
	struct sync_hdr* p_hdr;
	char title[128] = { 0 };
	volatile uint8_t* fenster_sync;
	struct fenster_input_data* inp;
	uint32_t* p_pixbuf;
	volatile int W, H;
	uint32_t hdr_sz = sizeof(struct sync_hdr);
	uint32_t inp_sz = sizeof(struct fenster_input_data);
	uint32_t size1 = hdr_sz + inp_sz;
	uint32_t size2;
	void* hdl1;
	void* hdl2;
	int fs = 0;

	printf("check shared mem 1\n");
	p_mem = (uint8_t*)create_sharedmem(mapfile1, size1, 0, &hdl1);
	if (!p_mem) { printf("Err -> exit\n"); return 1; }
	printf("OK\n");
	p_hdr = (struct sync_hdr*)p_mem;
	fenster_sync = &p_hdr->sync;
	W = p_hdr->w;
	H = p_hdr->h;
	inp = (struct fenster_input_data*)&p_mem[hdr_sz];
	printf("waiting for data\n");
	while (W * H == 0) { usleep(1000); }
	printf("OK: %dx%d\n", W, H);
	size2 = W * H * sizeof(int32_t);
	printf("check shared mem 2\n");
	p_pixbuf = (uint32_t*)create_sharedmem(mapfile2, size2, 0, &hdl2);
	printf("OK\n");
	
	strcpy_s(title, 128, (const char *)p_hdr->title);
	f.title = title;
	f.width = W;
	f.height = H;
	f.buf = p_pixbuf;
	f.allow_resize = 1;
	printf("window open\n");
	fenster_open(&f);
	printf("OK\n");
	now = fenster_time();
	while (fenster_loop(&f) == 0) {
		if (*fenster_sync == 2) break;
		if (*fenster_sync == 3) {
			int m = p_hdr->win_w * p_hdr->win_h;
			if (m > 0) {
				if (m == 1) {
					fs = 1 - fs;
					fenster_fullscreen(&f, fs);
				} else {
					fenster_resize(&f, p_hdr->win_w, p_hdr->win_h);
				}
				// p_hdr->win_w = p_hdr->win_h = 0;
			}
			*fenster_sync = 0;
		}
		*inp = f.inp;

		*fenster_sync = 1;
		while (*fenster_sync == 1) { fenster_sleep(1); }
		time = fenster_time();
		if (time - now < 1000 / 60) {
			fenster_sleep(time - now);
		}
		now = time;
	}
	*fenster_sync = 2;
	printf("window close\n");
	fenster_close(&f);
	printf("OK\n");

	destroy_sharedmem(p_mem, &hdl1);
	destroy_sharedmem(p_pixbuf, &hdl2);
	return 0;
}

