/* See gl_draw.h. Everything drawn in a frame goes through one shader and a
 * vertex batch (x, y, u, v, r, g, b, a); the batch is flushed when the kind
 * of drawing changes (plain colour, font, picture), before clipping, and at
 * the end of the frame. */
#include "gl_draw.h"

#include <GLES2/gl2.h>
#include <string.h>

#include "draw.h"
#include "rows.h"

extern unsigned char console_font_8x16[];  /* font_8x16.c: CP437, libogc2's */

#define FONT_UP   4                 /* font texture enlarged 4 times: smoothing only softens edges */
#define FONT_TW   (16 * 8 * FONT_UP)
#define FONT_TH   (16 * 16 * FONT_UP)
#define MAX_QUADS 2048

enum { MODE_COLOR, MODE_FONT, MODE_PICTURE };

static const char *VS =
    "attribute vec2 aPos;\n"
    "attribute vec2 aTex;\n"
    "attribute vec4 aCol;\n"
    "varying vec2 vTex;\n"
    "varying vec4 vCol;\n"
    "void main() {\n"
    "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "  vTex = aTex;\n"
    "  vCol = aCol;\n"
    "}\n";
static const char *FS =
    "precision mediump float;\n"
    "uniform sampler2D uTex;\n"
    "uniform float uMode;\n"
    "varying vec2 vTex;\n"
    "varying vec4 vCol;\n"
    "void main() {\n"
    "  if (uMode < 0.5) gl_FragColor = vCol;\n"
    "  else if (uMode < 1.5) gl_FragColor = vec4(vCol.rgb, vCol.a * texture2D(uTex, vTex).a);\n"
    "  else gl_FragColor = vec4(texture2D(uTex, vTex).rgb, 1.0);\n"
    "}\n";

static GLuint program, fontTex, screenTex, largeTex, fbo;
static GLint aPos, aTex, aCol, uMode;
static int surfW, surfH;          /* surface */
static int vx, vy, vw, vh;        /* canvas on the surface, in pixels */
static int k = 1;                 /* whole enlargement of the screen copy */
static int largeK;                /* k of the current largeTex (0: none) */
static bool twoSteps;
static float verts[MAX_QUADS * 6 * 8];
static int nverts;
static int mode = MODE_COLOR;
static GLuint boundTex;

static GLuint shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

static GLuint texture(GLint filter)
{
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

void gl_init(void)
{
    program = glCreateProgram();
    glAttachShader(program, shader(GL_VERTEX_SHADER, VS));
    glAttachShader(program, shader(GL_FRAGMENT_SHADER, FS));
    glLinkProgram(program);
    aPos = glGetAttribLocation(program, "aPos");
    aTex = glGetAttribLocation(program, "aTex");
    aCol = glGetAttribLocation(program, "aCol");
    uMode = glGetUniformLocation(program, "uMode");

    /* Font: 16 x 16 glyphs of 8 x 16 points, each point FONT_UP x FONT_UP */
    static unsigned char alpha[FONT_TW * FONT_TH];
    memset(alpha, 0, sizeof(alpha));
    for (int g = 0; g < 256; g++) {
        int gx = (g % 16) * 8, gy = (g / 16) * 16;
        for (int r = 0; r < 16; r++) {
            unsigned char bits = console_font_8x16[g * 16 + r];
            for (int c = 0; c < 8; c++) {
                if (!(bits & (0x80 >> c)))
                    continue;
                for (int dy = 0; dy < FONT_UP; dy++)
                    memset(alpha + ((gy + r) * FONT_UP + dy) * FONT_TW + (gx + c) * FONT_UP, 255, FONT_UP);
            }
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    fontTex = texture(GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_TW, FONT_TH, 0, GL_ALPHA, GL_UNSIGNED_BYTE, alpha);
    screenTex = texture(GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_W, SCR_H, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    largeTex = 0;
    fbo = 0;
    largeK = 0;
    surfW = surfH = 0;
}

/* --- batch ------------------------------------------------------------------- */

static void flush(void)
{
    if (nverts == 0)
        return;
    glUseProgram(program);
    glUniform1f(uMode, (float)mode);
    if (mode != MODE_COLOR) {
        glBindTexture(GL_TEXTURE_2D, boundTex);
    }
    glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), verts);
    glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), verts + 2);
    glVertexAttribPointer(aCol, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), verts + 4);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aTex);
    glEnableVertexAttribArray(aCol);
    glDrawArrays(GL_TRIANGLES, 0, nverts);
    nverts = 0;
}

static void use(int m, GLuint tex)
{
    if (m != mode || (m != MODE_COLOR && tex != boundTex) || nverts + 6 > MAX_QUADS * 6) {
        flush();
        mode = m;
        boundTex = tex;
    }
}

/* Quad in normalised device coordinates (x0, y0 top-left), texture u0..u1, v0..v1 */
static void quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, rgba_t c)
{
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f, a = c.a / 255.0f;
    float q[6][4] = {{x0, y0, u0, v0}, {x1, y0, u1, v0}, {x0, y1, u0, v1},
                     {x1, y0, u1, v0}, {x1, y1, u1, v1}, {x0, y1, u0, v1}};
    for (int i = 0; i < 6; i++) {
        float *v = verts + nverts * 8;
        v[0] = q[i][0]; v[1] = q[i][1]; v[2] = q[i][2]; v[3] = q[i][3];
        v[4] = r; v[5] = g; v[6] = b; v[7] = a;
        nverts++;
    }
}

static float nx(float x) { return x / CANVAS_W * 2 - 1; }
static float ny(float y) { return 1 - y / CANVAS_H * 2; }

/* --- draw.h ------------------------------------------------------------------ */

void draw_fill(float x, float y, float w, float h, rgba_t c)
{
    if (w <= 0 || h <= 0)
        return;
    use(MODE_COLOR, 0);
    quad(nx(x), ny(y), nx(x + w), ny(y + h), 0, 0, 0, 0, c);
}

void draw_glyphs(float x, float y, float scale, rgba_t c, const char *cp437, int len)
{
    use(MODE_FONT, fontTex);
    float gw = GLYPH_W * scale, gh = GLYPH_H * scale;
    for (int i = 0; i < len; i++) {
        unsigned char g = (unsigned char)cp437[i];
        if (g == ' ') {
            x += gw;
            continue;
        }
        float u0 = (g % 16) * 8.0f / 128.0f, v0 = (g / 16) * 16.0f / 256.0f;
        if (nverts + 6 > MAX_QUADS * 6)
            flush();
        quad(nx(x), ny(y), nx(x + gw), ny(y + gh), u0, v0, u0 + 8.0f / 128.0f, v0 + 16.0f / 256.0f, c);
        x += gw;
    }
}

void draw_clip(float x, float y, float w, float h)
{
    flush();
    float s = (float)vw / CANVAS_W;
    int px = vx + (int)(x * s), pw = (int)(w * s + 0.5f);
    int ph = (int)(h * s + 0.5f);
    int py = vy + vh - (int)(y * s) - ph;   /* GL counts from the bottom */
    glEnable(GL_SCISSOR_TEST);
    glScissor(px, py, pw, ph);
}

void draw_noclip(void)
{
    flush();
    glDisable(GL_SCISSOR_TEST);
}

/* --- frame ------------------------------------------------------------------- */

void gl_begin(int w, int h)
{
    if (w != surfW || h != surfH) {
        surfW = w;
        surfH = h;
        float s = (float)w / CANVAS_W < (float)h / CANVAS_H ? (float)w / CANVAS_W : (float)h / CANVAS_H;
        vw = (int)(CANVAS_W * s + 0.5f);
        vh = (int)(CANVAS_H * s + 0.5f);
        vx = (w - vw) / 2;
        vy = (h - vh) / 2;
        GLint max = 2048;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
        k = vw / SCR_W > 0 ? vw / SCR_W : 1;
        while (k > 1 && SCR_W * k > max)
            k--;
    }
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, surfW, surfH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(vx, vy, vw, vh);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    nverts = 0;
    mode = MODE_COLOR;
}

void gl_screen(const uint16_t *pixels, bool changed)
{
    rgba_t white = {255, 255, 255, 255};
    flush();
    if (largeK != k) {   /* (re)make the enlarged texture for this size */
        if (largeTex) {
            glDeleteTextures(1, &largeTex);
            glDeleteFramebuffers(1, &fbo);
        }
        largeTex = texture(GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_W * k, SCR_H * k, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, largeTex, 0);
        twoSteps = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        largeK = k;
        changed = true;
    }
    if (changed) {
        glBindTexture(GL_TEXTURE_2D, screenTex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_W, SCR_H, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
    }
    if (twoSteps) {
        if (changed) {
            glDisable(GL_BLEND);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, SCR_W * k, SCR_H * k);
            use(MODE_PICTURE, screenTex);
            quad(-1, 1, 1, -1, 0, 0, 1, 1, white);   /* first row at the top... */
            flush();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(vx, vy, vw, vh);
            glEnable(GL_BLEND);
        }
        use(MODE_PICTURE, largeTex);
        quad(-1, 1, 1, -1, 0, 1, 1, 0, white);       /* ...so the enlarged copy is upside down */
    } else {
        use(MODE_PICTURE, screenTex);
        quad(-1, 1, 1, -1, 0, 0, 1, 1, white);
    }
    flush();
}

void gl_end(void)
{
    flush();
    glDisable(GL_SCISSOR_TEST);
}

bool gl_canvas_point(float sx, float sy, float *cx, float *cy)
{
    if (vw <= 0 || vh <= 0)
        return false;
    float top = (float)(surfH - vy - vh);   /* vy counts from the bottom */
    *cx = (sx - (float)vx) * CANVAS_W / (float)vw;
    *cy = (sy - top) * CANVAS_H / (float)vh;
    return *cx >= 0 && *cx < CANVAS_W && *cy >= 0 && *cy < CANVAS_H;
}
