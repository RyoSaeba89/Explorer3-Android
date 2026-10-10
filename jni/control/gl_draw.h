/* Mission control on Android: OpenGL ES 2 implementation of GC-Explorer3's
 * draw.h (filled rectangles, 8x16 font, clipping) plus the copy of the
 * crew's screen. The 960 x 540 canvas is scaled to fit the screen, aspect
 * kept (user's choice: scale to fit, sharp at any resolution). GL thread
 * only. */
#ifndef GL_DRAW_H
#define GL_DRAW_H
#include <stdbool.h>
#include <stdint.h>

/* The GL context was (re)created: shaders and textures. */
void gl_init(void);

/* Start a frame on a w x h surface: black, canvas placed and scaled. */
void gl_begin(int w, int h);

/* The crew's 240 x 135 screen (RGB565 values) over the whole canvas,
 * enlarged a whole number of times without smoothing, then smoothed to the
 * final size: big pixels stay square and sharp. changed: new pixels. */
void gl_screen(const uint16_t *pixels, bool changed);

/* Finish the frame. */
void gl_end(void);

#endif
