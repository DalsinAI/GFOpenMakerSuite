/* omview_window.h on the Amiga.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_window.h"

#include <graphics/text.h>
#include <proto/intuition.h>

struct Window *omv_window_open(const char *title, ULONG idcmp)
{
    struct Screen *s = LockPubScreen(NULL);
    struct Window *w;
    int sw = 640, sh = 512, top = 11, bw = 8, bh = 21, iw, ih;
    if (s) {
        sw = s->Width;
        sh = s->Height;
        /* It opens below the screen's title bar and fills the rest. */
        top = s->BarHeight + 1;
        /* The title bar is the screen font's height and a line; the bottom
         * border holds the sizing gadget, about 10 pixels. */
        bw = s->WBorLeft + s->WBorRight;
        bh = s->WBorTop + (s->Font ? s->Font->ta_YSize : 8) + 1 + 10;
    }
    omv_window_fit(sw, sh, top, bw, bh, &iw, &ih);
    w = OpenWindowTags(NULL,
        WA_Title, (ULONG)title,
        WA_PubScreen, (ULONG)s,
        WA_Left, 0UL,
        WA_Top, (ULONG)top,
        WA_InnerWidth, (ULONG)iw,
        WA_InnerHeight, (ULONG)ih,
        WA_MaxWidth, (ULONG)sw,
        WA_MaxHeight, (ULONG)sh,
        WA_AutoAdjust, TRUE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_SizeBBottom, TRUE,
        WA_IDCMP, idcmp,
        WA_ReportMouse, TRUE,
        WA_Activate, TRUE,
        TAG_END);
    if (s) UnlockPubScreen(NULL, s);
    /* Its real borders known now: no smaller than the least inside, no
     * bigger than the screen. */
    if (w)
        WindowLimits(w, (WORD)(OMV_MIN_W + w->BorderLeft + w->BorderRight),
                     (WORD)(OMV_MIN_H + w->BorderTop + w->BorderBottom), (UWORD)sw, (UWORD)sh);
    return w;
}
