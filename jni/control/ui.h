/* Mission control on Android: the screens before the stream (Android's own,
 * in the style of mission control's pages, drawn through draw.h). The
 * word "Cardputer" appears nowhere (user's rule): the crew may be a
 * Cardputer or an Android device. Buttons are drawn as the device's own
 * (PlayStation symbols on the Xperia Play, O U Y A on the OUYA); without a
 * gamepad, the hints are framed words to tap. Hints, crews and the address
 * can be tapped on every device (ui_tap). */
#ifndef UI_H
#define UI_H
#include <stdbool.h>
#include <stdint.h>

#include "addr.h"
#include "net.h"

/* Why connecting failed */
enum {
    NET_REASON_NONE,
    NET_REASON_UNREACHABLE,  /* nothing answers (or not within 30 s) */
    NET_REASON_REFUSED,      /* a device answers, but nothing listens on the port */
    NET_REASON_NOT_GAME,     /* the port answers, but not as the game's WebSocket */
    NET_REASON_NO_SOCKET,    /* no socket left */
    NET_REASON_LOST,         /* connected, then lost */
};

enum {
    UI_NO_NETWORK,   /* Android is not connected */
    UI_SEARCH,       /* looking for the crew on the local network */
    UI_NONE,         /* search over, nothing found */
    UI_LIST,         /* several crews found: choose */
    UI_ADDRESS,      /* typed address */
    UI_CONNECTING,   /* checking an address, or connecting */
    UI_ERROR,
};

/* Buttons the hints show (Device.BUTTONS in Java) */
enum { PAD_PLAYSTATION, PAD_OUYA, PAD_TOUCH };

/* What a tap on a screen does (ui_tap) */
enum {
    TAP_NONE,
    TAP_OK,        /* the hint of that button */
    TAP_BACK,
    TAP_KB,
    TAP_CREW,      /* a crew of the list: arg = its index */
    TAP_ADDRESS,   /* the address: Android's number keyboard */
};

typedef struct {
    int      screen;
    int      lang;           /* 0 French, 1 English */
    int      pad;            /* PAD_... */
    uint32_t now, since;
    char     net_name[64];   /* Wi-Fi name, or "Ethernet" */
    uint8_t  self[4];
    int      progress;       /* search, 0..100 */
    found_t  found[FINDER_MAX];
    int      nfound;
    int      sel;            /* LIST */
    addr_entry_t entry;      /* ADDRESS */
    uint8_t  target[4];      /* CONNECTING, ERROR */
    int      reason;         /* ERROR: NET_REASON_... */
} ui_t;

void ui_draw(const ui_t *u);

/* While the stream is shown: how to leave it, bottom right, for a few
 * seconds after it starts. */
void ui_stream_hint(int lang, int pad);

/* Over the stream, bottom left, as the web page's #etat: "Connecting to
 * the crew..." while reconnecting, and a warning when the crew's game is
 * another version (pages refused). */
void ui_overlay(int lang, bool reconnecting, bool old_game);

/* What can be tapped is noted while drawing: ui_taps_clear() before a frame,
 * then ui_tap() says what is at (x, y) of the canvas (TAP_..., arg for
 * TAP_CREW). */
void ui_taps_clear(void);
int ui_tap(float x, float y, int *arg);

#endif
