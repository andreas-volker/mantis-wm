#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>

#define MIN(X, Y)       ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)       ((X) > (Y) ? (X) : (Y))
#define ROUND(X)        (((double)X) + 0.9999999) /* prevents truncation */
#define CLAMP(X, A, Z)  (((X) > (Z)) ? (Z) : (((X) < (A)) ? (A) : (X)))
#define LEN(X)          (sizeof (X) / sizeof ((X)[0]))
#define LOCK            (LockMask|Mod2Mask|Mod3Mask) /* caps, num, scroll */
#define MOD             (ShiftMask|ControlMask|Mod1Mask|Mod4Mask|Mod5Mask)

#define SINK     (1<<0)
#define NORMAL   (1<<1)
#define FLOAT    (1<<2)
#define FULL     (1<<3)
#define URGENT   (1<<4)
#define INPUT    (1<<5)
#define TYPEMASK (SINK|NORMAL|FLOAT|FULL|URGENT|INPUT)

typedef struct list {
    struct list *prev, *next;
    void        *ptr;
} List;

typedef struct {
    Window          win;
    int             x, y;
    unsigned int    w, h, type, newtype;
    unsigned long   tag;
} Win;

typedef struct {
    int     i;
    char    *c;
} Arg;

typedef struct {
    char            *id;
    unsigned int    mod;
    KeySym          key;
    void            (*func)(Win*, Arg*(*)[2]);
    Arg             arg[2];
} Key;

typedef struct {
    unsigned int    mod, button, arg;
} Button;

typedef struct {
    char    *name;
    Bool    tile;
    List    *data;
} Layout;

typedef struct {
    int             x, y;
    unsigned int    width, height, col, row;
} Laydata;

typedef struct {
    Window  win;
} Sink;

struct {
    Display         *dpy;
    Bool            run, keypress;
    int             screen, width, height, ret;
    Window          root, chld, current, prev, empty, lastsink;
    Colormap        colormap;
    unsigned int    dwidth, dheight;
    unsigned long   tag, tagmask, border[3];
    List            *wins, *sinks, *stack, *keys, *buttons, *layouts;
} data;

struct {
    char            *border_in, *border_out, *border_urgent, *sinks, *floats,
                    *layout;
    int             gap_win, gap_top, gap_left, gap_right, gap_bottom,
                    border_width, tags, focus, newwin, follow_mouse;
    unsigned int    follow_mod;
} var;

/* button.c */
static char *button_add(char*, char*);
static void button_free(void);
static void button_func(Win*, unsigned int);
static void button_grab(Window);
static void moveresizemouse(Win*, unsigned int);
/* event.c */
static void buttonpress(XEvent*);
static void clientmessage(XEvent*);
static void configurerequest(XEvent*);
static void enternotify(XEvent*);
static void keypress(XEvent*);
static void keyrelease(XEvent*);
static void mapnotify(XEvent*);
static void mappingnotify(XEvent*);
static void maprequest(XEvent*);
static void propertynotify(XEvent*);
static void unmapdestroynotify(XEvent*);
/* ewmh.c */
static void ewmh_init(void);
static void ewmh_area(void);
static Bool ewmh_atoms(Window, Atom, Atom**);
static void ewmh_change(Window, Atom, Atom, int);
static void ewmh_check(Window, unsigned int*);
static void ewmh_frame(void);
static void ewmh_list(void);
static char *ewmh_str(int);
/* icccm.c */
static void icccm_init(void);
static void icccm_check(Window, unsigned int*);
static Bool icccm_sendmsg(Window, Atom);
static void icccm_size(Win*);
static char *icccm_str(int);
/* key.c */
static char *key_add(char*, char*, char*(*)[2], char(*)[2]);
static void key_free(void);
static void key_grab(void);
static void keyclose(Win*, Arg*(*)[2]);
static void keyfocus(Win*, Arg*(*)[2]);
static void keykill(Win*, Arg*(*)[2]);
static void keymacro(Win*, Arg*(*)[2]);
static void keymove(Win*, Arg*(*)[2]);
static void keyquit(Win*, Arg*(*)[2]);
static void keyresize(Win*, Arg*(*)[2]);
static void keyrestart(Win*, Arg*(*)[2]);
static void keyset(Win*, Arg*(*)[2]);
static void keyspawn(Win*, Arg*(*)[2]);
static void keyswitch(Win*, Arg*(*)[2]);
static void keytag(Win*, Arg*(*)[2]);
static void keytoggle(Win*, Arg*(*)[2]);
/* layout.c */
static char *layout_add(char*, char*, int);
static void layout_arrange(void);
static void layout_free(void);
static Layout *layout_get(char*);
static void layout_resize(int, int);
static void resize(int, int, int*, int*, int);
static void tile(Win*, int, int, unsigned int, unsigned int, unsigned int,
                 unsigned int);
/* list.c */
static List *list_find(List*, void*);
static List *list_first(List*);
static int list_index(List*, void*);
static List *list_insert(List*, void*, int);
static List *list_last(List*);
static unsigned int list_len(List*);
static List *list_nth(List*, unsigned int);
static List *list_remove(List*, void*);
/* main.c */
static void clean(void);
static int error(Display*, XErrorEvent*);
static void init(void);
/* sink.c */
static void sink_add(Window);
static void sink_del(Window);
static void sink_free(void);
static Window sink_last(void);
static void sink_update(void);
static Sink *getsink(Window);
/* rc.c */
static void rc_init(void);
static void rc_mask(char*, unsigned int*, char**);
static unsigned int getmodmask(char*);
static unsigned int getmodnumber(char);
static char *parselayout(char*);
static char *parseline(char*);
static char *parsepath(char*);
static char *setparsed(char*, char**);
/* util.c */
static void *emalloc(size_t);
static void eprintf(char*, ...);
static char *estrdup(char*);
/* var.c */
static void var_init(void);
static void var_free(void);
static char *var_set(char*, char**);
/* win.c */
static void win_close(Window);
static void win_focus(Window);
static void win_manage(Window);
static void win_map(Window);
static void win_resize(Win*);
static void win_stackreorder(void);
static void win_toggle(Win*);
static void win_unmanage(Win*);
static void win_updateborder(void);
static void win_updateinfo(void);
static void win_updatetype(Win*);
static Bool win_first(unsigned long, unsigned int, List**, Win**);
static Win *win_get(Window);
static int win_index(unsigned long, unsigned int, Window);
static Bool win_last(unsigned long, unsigned int, List **, Win **);
static unsigned int win_len(unsigned long, unsigned int);
static void win_neighbor(unsigned long, unsigned int, Window, int, Window, int,
                         int, int*, Win**);
static Bool win_next(unsigned long, unsigned int, List**, Win**);
static Bool win_nth(unsigned long, unsigned int, List**, Win**, unsigned int);
static Bool win_prev(unsigned long, unsigned int, List**, Win**);

#include "config.h"
#include "util.c"
#include "list.c"
#include "var.c"
#include "ewmh.c"
#include "icccm.c"
#include "sink.c"
#include "win.c"
#include "layout.c"
#include "event.c"
#include "key.c"
#include "button.c"
#include "rc.c"

void
clean(void) {
    Win *w;
    List *l;
    unsigned int i;

    button_free();
    key_free();
    var_free();
    layout_free();
    sink_free();
    XFreeColors(data.dpy, data.colormap, data.border, 3, 0);
    for(i = 0; i < LASTEwmh; i++)
        XDeleteProperty(data.dpy, data.root, ewmh[i]);
    XDeleteProperty(data.dpy, data.root, XInternAtom(data.dpy, "_MANTIS_TAG",
                                                     False));
    XDeleteProperty(data.dpy, data.root, XInternAtom(data.dpy, "_MANTIS_VIEW",
                                                     False));
    for(l = list_first(data.wins); l;) {
        w = (Win*)l->ptr;
        l = l->next;
        XSetWindowBorderWidth(data.dpy, w->win, 0);
        XSelectInput(data.dpy, w->win, NoEventMask);
        XUngrabButton(data.dpy, AnyButton, AnyModifier, w->win);
        if(!(w->tag & data.tag))
            XMoveWindow(data.dpy, w->win, w->x, w->y);
        data.wins = list_remove(data.wins, w);
        data.stack = list_remove(data.stack, w);
        free(w);
    }
    XSelectInput(data.dpy, data.root, NoEventMask);
    XUngrabKey(data.dpy, AnyKey, AnyModifier, data.root);
    XSetInputFocus(data.dpy, PointerRoot, RevertToNone, CurrentTime);
    XDestroyWindow(data.dpy, data.chld);
    XSync(data.dpy, False);
    XCloseDisplay(data.dpy);
}

int
error(Display *d, XErrorEvent *e) {
    char req[32], buf[BUFSIZ];

    if(!data.run || e->error_code == BadWindow)
        return 0;
    if(e->request_code == X_ChangeWindowAttributes &&
       e->error_code == BadAccess)
        fputs(PROGRAM": Another window manager is already running.\n", stderr);
    else if(e->request_code == X_GrabKey && e->error_code == BadAccess)
        fputs(PROGRAM": Another program already grabbed the keys.\n", stderr);
    else {
        sprintf(req, "%d", e->request_code);
        XGetErrorDatabaseText(d, "XRequest", req, "", buf, BUFSIZ);
        fprintf(stderr, PROGRAM": request %s:", buf);
        XGetErrorText(d, e->error_code, buf, BUFSIZ);
        fprintf(stderr, " %s.\n", buf);
    }
    data.ret = EXIT_FAILURE;
    data.run = False;
    return 0;
}

void
init(void) {
    Window dw, *wins = NULL;
    unsigned int i, len;
    XWindowAttributes a;

    data.current = data.prev = data.empty = data.lastsink = None;
    data.border[0] = data.border[1] = data.border[2] = 0L;
    data.wins = data.sinks = data.stack = NULL;
    data.keys = data.buttons = data.layouts = NULL;
    data.screen     = DefaultScreen(data.dpy);
    data.dwidth     = DisplayWidth(data.dpy, data.screen);
    data.dheight    = DisplayHeight(data.dpy, data.screen);
    data.root       = RootWindow(data.dpy, data.screen);
    data.colormap   = DefaultColormap(data.dpy, data.screen);
    data.run        = True;
    data.ret        = EXIT_SUCCESS;
    data.tag        = 1<<0;
    var_init();
    rc_init();
    XSetErrorHandler(error);
    XSelectInput(data.dpy, data.root, SubstructureRedirectMask|
                 StructureNotifyMask|SubstructureNotifyMask);
    key_grab();
    ewmh_init();
    icccm_init();
    if(data.run && XQueryTree(data.dpy, data.root, &dw, &dw, &wins, &len))
        for(i = 0; i < len; i++)
            if(XGetWindowAttributes(data.dpy, wins[i], &a)) {
                if(!a.override_redirect && a.map_state == IsViewable)
                    win_manage(wins[i]);
                else if(a.override_redirect && a.map_state == IsViewable)
                    sink_add(wins[i]);
            }
    if(wins) {
        win_focus(wins[0]);
        XFree(wins);
        layout_arrange();
    }
}

int
main(int argc, char **argv) {
    XEvent e;
    (void)argv;

    if(argc != 1)
        eprintf(PROGRAM": version: "VERSION"\n");
    else if(!(data.dpy = XOpenDisplay(NULL)))
        eprintf(PROGRAM": Can not open display.\n");
    init();
    while(data.run) {
        XNextEvent(data.dpy, &e);
        if(event[e.type])
            event[e.type](&e);
    }
    clean();
    return data.ret;
}
