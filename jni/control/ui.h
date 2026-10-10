/* Mission control on Android: the screens before the stream (Android's own,
 * in the style of mission control's pages, drawn through draw.h). The
 * word "Cardputer" appears nowhere (user's rule): the crew may be a
 * Cardputer or an Android device. Buttons are drawn as the device's own
 * (PlayStation symbols on the Xperia Play, O U Y A on the OUYA). */
#ifndef UI_H
#define UI_H
#include <stdbool.h>
#include <stdint.h>

#include "addr.h"
#include "net.h"

enum {
    UI_NO_NETWORK,   /* Android is not connected */
    UI_SEARCH,       /* looking for the crew on the local network */
    UI_NONE,         /* search over, nothing found */
    UI_LIST,         /* several crews found: choose */
    UI_ADDRESS,      /* typed address */
    UI_CONNECTING,   /* checking an address, or connecting */
    UI_ERROR,
};

typedef struct {
    int      screen;
    int      lang;           /* 0 French, 1 English */
    bool     ouya;           /* buttons drawn as the OUYA's */
    uint32_t now, since;
    char     net_name[64];   /* Wi-Fi name, or "Ethernet" */
    uint8_t  self[4];
    int      progress;       /* search, 0..100 */
    found_t  found[FINDER_MAX];
    int      nfound;
    int      sel;            /* LIST */
    addr_entry_t entry;      /* ADDRESS */
    uint8_t  target[4];      /* CONNECTING, ERROR */
    int      reason;         /* ERROR: NET_REASON_... (netui.h) */
} ui_t;

void ui_draw(const ui_t *u);

/* While the stream is shown: how to leave it, bottom right, for a few
 * seconds after it starts. */
void ui_stream_hint(int lang, bool ouya);

#endif
