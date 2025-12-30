# Fensterl

Fenster /ˈfɛnstɐ/ -- a German word for "window".

Fensterl /ˈfɛnstɐl/ -- a Bavarian (South-German dialect) word for "tiny window".

This library is a direct derivate of "Fenster" (https://github.com/zserge/fenster), but with several bugfixes and new features added by me and other authors (most notably the open pull requests from https://github.com/zserge/fenster/pulls) as well as the addons by Vaguiner Gonzalez dos Santos from here: https://github.com/vaguinerg/fensterb.

What is it?
This library provides a small and highly opinionated way to display a cross-platform 2D canvas. If you remember Borland BGI or drawing things in QBASIC or `INT 10h`- you know what I mean. As a nice bonus you also get cross-platform keyboard/mouse input and audio playback in only a few lines of code.

## What it does for you

* Single application window of given size with a title.
* Application lifecycle and system events are all handled automatically.
* Minimal 24-bit RGB framebuffer.
* Cross-platform keyboard events (keycodes).
* Cross-platform mouse events (X/Y + mouse click).
* Cross-platform timers to have a stable FPS rate.
* Cross-platform audio playback (WinMM, CoreAudio, ALSA).
* Simple polling API without a need for callbacks or multithreading (like Arduino/Processing).
* One C99 header of ~300LOC, easy to understand and extend.
* And, yes, [it can run Doom](/examples/doom-c)!

## Example

Here's how to draw white noise:

```c
// main.c
#include "fenster.h"
#define W 320
#define H 240
int main() {
  uint32_t buf[W * H];
  struct fenster f = { .title = "hello", .width = W, .height = H, .buf = buf };
  fenster_open(&f);
  while (fenster_loop(&f) == 0) {
    for (int i = 0; i < W; i++) {
      for (int j = 0; j < H; j++) {
        fenster_pixel(&f, i, j) = rand();
      }
    }
  }
  fenster_close(&f);
  return 0;
}
```

Compile it and run:

```
# Linux
cc main.c -lX11 -lasound -o main && ./main
# macOS
cc main.c -framework Cocoa -framework AudioToolbox -o main && ./main
# windows
cc main.c -lgdi32 -lwinmm -o main.exe && main.exe
```

That's it.

## API

API is designed to be a polling loop, where on every iteration the framebuffer get updated and the user input (mouse/keyboard) can be polled.

```c
struct fenster_input_data {
	uint8_t key_down[256];          /* keys are mostly ASCII, but arrows are 17..20 (persistent until release) */
	uint8_t key[256];               /* like key_down, but one press signal only (one click) */
	uint8_t key_mod[4];             /* ctrl, shift, alt, meta */
	uint32_t mouse_pos[2];          /* mouse x, y */
	uint8_t mouse_button[5];        /* left, right, middle, scroll up, scroll down (one click) */
	uint8_t mouse_button_down[3];   /* left, right, middle (persistent until release) */
};

struct fenster {
	const char* title; /* window title */
	int width; /* pixel buffer width */
	int height; /* pixel buffer height */
	uint32_t* buf; /* actual pixel buffer, 24-bit RGB, row by row, pixel by pixel */
	struct fenster_input_data inp; /* see structure above */
	uint8_t allow_resize; /* set this to 1 to allow window resizing */
	int win_width; /* size of the window */
	int win_height; /* height of the window */
	int64_t last_sync;   // last sync time
};

```

* `int fenster_open(struct fenster *f)` - opens a new app window.
* `int fenster_loop(struct fenster *f)` - handles system events and refreshes the canvas. Returns negative values when app window is closed.
* `void fenster_close(struct fenster *f)` - closes the window and exists the graphical app.
* `void fenster_sleep(int ms)` - pauses for `ms` milliseconds.
* `int64_t fenster_time()` - returns current time in milliseconds.
* `fenster_pixel(f, x, y) = 0xRRGGBB` - set pixel color.
* `uint32_t px = fenster_pixel(f, x, y);` - get pixel color.

See [examples/drawing-c](/examples/drawing-c) for more old-school drawing primitives, but also feel free to experiment with your own graphical algorithms!

## License

Code is distributed under MIT license, feel free to use it in your proprietary projects as well.
