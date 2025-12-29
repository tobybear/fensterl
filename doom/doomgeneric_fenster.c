#include "../sharedmem.h"
#include "../fenster_sync.h"
#include "doomgeneric.h"
#include "doomkeys.h"

#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef __COSMOCC__
#include "include/libc/dce.h"
__static_yoink("zip_uri_support");
#else
#ifdef _WIN32
static void usleep(__int64 usec) {
	HANDLE timer;
	LARGE_INTEGER period;
	period.QuadPart = -(10 * usec);
	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &period, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}
#endif
#endif

int i_main(int argc, char **argv);

struct fenster_input_data {
	uint8_t key_down[256];          // keys are mostly ASCII, but arrows are 17..20 (persisent until release)
	uint8_t key[256];               // like key_down, but one press signal only (one click)
	uint8_t key_mod[4];             // ctrl, shift, alt, meta
	uint32_t mouse_pos[2];          // mouse x, y
	uint8_t mouse_button[5];        // left, right, middle, scroll up, scroll down (one click)
	uint8_t mouse_button_down[3];   // left, right, middle (persistent until release)
};

uint8_t* p_mem;
struct sync_hdr* p_hdr;
volatile uint8_t* fenster_sync;
struct fenster_input_data* inp;
uint32_t* p_pixbuf;
uint32_t size1, size2;
void* hdl1;
void* hdl2;
const int W = DOOMGENERIC_RESX;
const int H = DOOMGENERIC_RESY;

void DG_Init() {
}

void DG_DrawFrame() { 
	while (*fenster_sync == 0) { usleep(1000); }

	memcpy(p_pixbuf, DG_ScreenBuffer, size2);
	if (inp->key_down[27] || *fenster_sync == 2) {
		*fenster_sync = 2;
		usleep(1000);
		destroy_sharedmem(p_mem, &hdl1);
		destroy_sharedmem(p_pixbuf, &hdl2);
		exit(0); 
	}
	*fenster_sync = 0;
	if (inp->key_mod[0] > 0) {
		if (inp->key['1']) { 
			p_hdr->win_w = W;
			p_hdr->win_h = H;
			*fenster_sync = 3;
		} else if (inp->key['2']) { 
			p_hdr->win_w = W * 2;
			p_hdr->win_h = H * 2;
			*fenster_sync = 3;
		} else if (inp->key['3']) { 
			p_hdr->win_w = W * 4;
			p_hdr->win_h = H * 4;
			*fenster_sync = 3;
		} else if (inp->key['4']) { 
			p_hdr->win_w = 1;
			p_hdr->win_h = 1;
			*fenster_sync = 3;
		}
	}
}

void DG_SleepMs(uint32_t ms) { 
	usleep(ms * 1000); 
}

uint32_t DG_GetTicksMs() {
#ifdef _WIN32
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return (int64_t)(count.QuadPart * 1000.0 / freq.QuadPart);
#else
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000 + (time.tv_nsec / 1000000);
#endif
}

void DG_SetWindowTitle(const char *title) { 
	(void)title;
}

unsigned char toDoomKey(int k) {
  switch (k) {
  case '\n':
    return KEY_ENTER;
  case '\x1b':
    return KEY_ESCAPE;
  case '\x11':
    return KEY_UPARROW;
  case '\x12':
    return KEY_DOWNARROW;
  case '\x13':
    return KEY_RIGHTARROW;
  case '\x14':
    return KEY_LEFTARROW;
  case 'Z':
    return KEY_FIRE;
  case 'Y':
    return KEY_FIRE;
  case 'X':
    return KEY_RSHIFT;
  case ' ':
    return KEY_USE;
  }
  return tolower(k);
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
  static int old[128] = {0};
  for (int i = 0; i < 128; i++) {
    if ((inp->key_down[i] && !old[i]) || (!inp->key_down[i] && old[i])) {
      *pressed = old[i] = inp->key_down[i];
      *doomKey = toDoomKey(i);
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
	const char* title = "CosmoDOOM";
	const char* mapfile1 = "/tmp/fstub";
	const char* mapfile2 = "/tmp/fstub_pix";
	uint32_t hdr_sz = sizeof(struct sync_hdr);
	uint32_t inp_sz = sizeof(struct fenster_input_data);
	size1 = hdr_sz + inp_sz;
	size2 = W * H * sizeof(int32_t);

	hdl1 = NULL;
	hdl2 = NULL;
	p_mem = (uint8_t*)create_sharedmem(mapfile1, size1, 1, &hdl1);
	if (p_mem == NULL) {
		printf("ERR1 -> exit\n");
		return 1;
	}
	memset(p_mem, 0, size1);	
	p_pixbuf = (uint32_t*)create_sharedmem(mapfile2, size2, 1, &hdl2);
	if (p_pixbuf == NULL) {
		printf("ERR2 -> exit\n");
		return 1;
	}
	memset(p_pixbuf, 0, size2);	
	
	p_hdr = (struct sync_hdr*)p_mem;
	memcpy(p_hdr->title, title, strlen(title));
	fenster_sync = &p_hdr->sync;
	p_hdr->w = W;
	p_hdr->h = H;
	inp = (struct fenster_input_data*)&p_mem[hdr_sz];
 	
	pid_t pid = fork();
    if (pid == 0) {
		char stub_path[128] = { 0 };
		if (IsWindows()) {
		  strcpy(stub_path, "/c/dev/fenster/fstub_front/x64/Debug/fstub_front.exe");//"/c/dev/fenster/doom/stub_win.exe");
	  	} else if (IsXnu()) {
		  strcpy(stub_path, "/c/dev/fenster/doom/stub_mac");
		} else {
		  strcpy(stub_path, "/c/dev/fenster/doom/stub_linux");
		}
printf("starting client %s\n", stub_path);
	    char *argv2[3] = { stub_path, "",  NULL };
        execvp(stub_path, argv2);
printf("OK\n");
		while (*fenster_sync != 2) { usleep(1000); }
    } else {
		printf("Waiting for framebuffer client...");
		while (*fenster_sync == 0) { usleep(1000); }
		printf("OK\n");

		i_main(argc, argv);
		destroy_sharedmem(p_mem, &hdl1);
		destroy_sharedmem(p_pixbuf, &hdl2);
	}
	return 0;
}
