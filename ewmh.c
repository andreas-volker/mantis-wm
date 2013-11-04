enum { _NET_SUPPORTED,
       _NET_CLIENT_LIST,
       _NET_CLIENT_LIST_STACKING,
       _NET_NUMBER_OF_DESKTOPS,
       _NET_DESKTOP_GEOMETRY,
       _NET_DESKTOP_VIEWPORT,
       _NET_CURRENT_DESKTOP,
       _NET_DESKTOP_NAMES,
       _NET_ACTIVE_WINDOW,
       _NET_WORKAREA,
       _NET_SUPPORTING_WM_CHECK,
       _NET_CLOSE_WINDOW,
       _NET_WM_NAME,
       _NET_WM_DESKTOP,
       _NET_WM_WINDOW_TYPE,
       _NET_WM_WINDOW_TYPE_DESKTOP,
       _NET_WM_WINDOW_TYPE_DOCK,
       _NET_WM_WINDOW_TYPE_TOOLBAR,
       _NET_WM_WINDOW_TYPE_MENU,
       _NET_WM_WINDOW_TYPE_UTILITY,
       _NET_WM_WINDOW_TYPE_SPLASH,
       _NET_WM_WINDOW_TYPE_DIALOG,
       _NET_WM_WINDOW_TYPE_NORMAL,
       _NET_WM_STATE,
       _NET_WM_STATE_MODAL,
       _NET_WM_STATE_STICKY,
       _NET_WM_STATE_MAXIMIZED_VERT,
       _NET_WM_STATE_MAXIMIZED_HORZ,
       _NET_WM_STATE_SKIP_TASKBAR,
       _NET_WM_STATE_SKIP_PAGER,
       _NET_WM_STATE_HIDDEN,
       _NET_WM_STATE_FULLSCREEN,
       _NET_WM_STATE_ABOVE,
       _NET_WM_STATE_BELOW,
       _NET_WM_STATE_DEMANDS_ATTENTION,
       _NET_WM_STRUT,
       _NET_WM_STRUT_PARTIAL,
       _NET_WM_PID,
       _NET_FRAME_EXTENTS,
       LASTEwmh
};

static Atom ewmh[LASTEwmh];

void
ewmh_area(void) {
    long l[4];

    memset(&l, 0, sizeof(l));
    l[2] = (data.width = data.dwidth - var.gap_left + var.gap_right);
    l[3] = (data.height = data.dheight - var.gap_top + var.gap_bottom);
    XChangeProperty(data.dpy, data.root, ewmh[_NET_WORKAREA], XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)&l, 4);
}

Bool
ewmh_atoms(Window win, Atom a, Atom **atoms) {
    int di;
    Atom da;
    unsigned char *p;
    unsigned long dl, i, n;

    *atoms = NULL;
    if(XGetWindowProperty(data.dpy, win, a, 0L, sizeof(Atom), False, XA_ATOM,
                          &da, &di, &n, &dl, &p) != Success)
        return False;
    *atoms = emalloc(sizeof(Atom) * (n + 1));
    for(i = 0; i < n; i++)
        (*atoms)[i] = *((Atom*)p + i);
    XFree(p);
    (*atoms)[i] = None;
    return True;
}

void
ewmh_change(Window win, Atom prop, Atom a, int action) {
    int i;
    Atom *atoms;
    Bool b;

    if(!win || !prop || !a || !ewmh_atoms(win, prop, &atoms))
        return;
    XDeleteProperty(data.dpy, win, prop);
    for(i = 0, b = False; atoms[i]; i++) {
        if(!b)
            b = a == atoms[i];
        if(a != atoms[i])
            XChangeProperty(data.dpy, win, prop, XA_ATOM, 32, PropModeAppend,
                            (unsigned char*)&(atoms[i]), 1);
    }
    if((b = action == 2 ? !b : action == 1))
        XChangeProperty(data.dpy, win, prop, XA_ATOM, 32, PropModeAppend,
                        (unsigned char*)&a, 1);
    free(atoms);
    /* _NET_WM_STATE_REMOVE 0
     * _NET_WM_STATE_ADD    1
     * _NET_WM_STATE_TOGGLE 2 */
}

void
ewmh_check(Window win, unsigned int *type) {
    char *c;
    Atom a[2], *atoms;
    unsigned int i, j, k;

    a[0] = ewmh[_NET_WM_WINDOW_TYPE];
    a[1] = ewmh[_NET_WM_STATE];
    *type &= TYPEMASK;
    for(i = 0; i < LEN(a); i++) {
        if(!ewmh_atoms(win, a[i], &atoms))
            continue;
        for(j = 0; atoms[j]; j++)
            for(k = 0; k < LASTEwmh; k++) {
                if(atoms[j] != ewmh[k])
                    continue;
                c = ewmh_str(k);
                c = strstr(c, "_NET_WM_WINDOW_TYPE_") ? c + 20 :
                    (strstr(c, "_NET_WM_STATE_") ? c + 14 : NULL);
                if(!c)
                    break;
                if(strstr(var.sinks, c)) {
                    *type |= SINK;
                    return;
                } else if(!(*type & NORMAL) && strstr(var.floats, c))
                    *type |= FLOAT;
                if(k == _NET_WM_STATE_DEMANDS_ATTENTION)
                    *type |= URGENT;
                if(k == _NET_WM_STATE_FULLSCREEN)
                    *type |= FULL;
            }
        free(atoms);
    }
}

void
ewmh_frame(void) {
    Win *w;
    List *l;
    long d[4];
    char *v[3];
    XColor c;
    unsigned int i;

    v[0] = var.border_in;
    v[1] = var.border_out;
    v[2] = var.border_urgent;
    for(i = 0; i < LEN(data.border); i++) {
        if(data.border[i])
            XFreeColors(data.dpy, data.colormap, &data.border[i], 1, 0);
        if(!XAllocNamedColor(data.dpy, data.colormap, v[i], &c, &c)) {
            fprintf(stderr, PROGRAM": Can not parse color: \"%s\".\n", v[i]);
            XAllocNamedColor(data.dpy, data.colormap, "black", &c, &c);
        }
        data.border[i] = c.pixel;
    }
    for(l = list_first(data.wins); l; l = l->next) {
        w = (Win*)l->ptr;
        XSetWindowBorderWidth(data.dpy, w->win, var.border_width);
    }
    d[0] = d[1] = d[2] = d[3] = var.border_width;
    XChangeProperty(data.dpy, data.root, ewmh[_NET_FRAME_EXTENTS], XA_CARDINAL,
                    32, PropModeReplace, (unsigned char*)&d, 4);
    win_updateborder();
}

void
ewmh_free(void) {
    unsigned int i;

    for(i = 0; i < LASTEwmh; i++)
        XDeleteProperty(data.dpy, data.root, ewmh[i]);
}

void
ewmh_init(void) {
    int i;
    Atom a;
    char *c;
    long l[4];

    a = XInternAtom(data.dpy, "UTF8_STRING", False);
    data.chld = XCreateWindow(data.dpy, data.root, 0, 0, 1, 1, 0,
                              CopyFromParent, InputOnly, CopyFromParent, 0L,
                              NULL);
    for(i = 0; i < LASTEwmh; i++) {
        ewmh[i] = XInternAtom(data.dpy, ewmh_str(i), False);
        XDeleteProperty(data.dpy, data.root, ewmh[i]);
    }
    c = PROGRAM;
    i = strlen(c) + 1;
#define EWMH(A, B, C, D, E, F) (XChangeProperty(data.dpy, (A), ewmh[(B)], (C),\
                               (D), PropModeReplace, (unsigned char*)(E), (F)))
    EWMH(data.root, _NET_DESKTOP_NAMES,       a, 8, c, i);
    EWMH(data.root, _NET_WM_NAME,             a, 8, c, i);
    EWMH(data.chld, _NET_WM_NAME,             a, 8, c, i);
    EWMH(data.root, _NET_SUPPORTING_WM_CHECK, XA_WINDOW,   32, &data.chld, 1);
    EWMH(data.chld, _NET_SUPPORTING_WM_CHECK, XA_WINDOW,   32, &data.chld, 1);
    EWMH(data.root, _NET_SUPPORTED,           XA_ATOM,     32, ewmh, LASTEwmh);
    memset(&l, 0, sizeof(l));
    EWMH(data.root, _NET_DESKTOP_VIEWPORT,    XA_CARDINAL, 32, &l, 2);
    EWMH(data.root, _NET_CURRENT_DESKTOP,     XA_CARDINAL, 32, &l, 1);
    l[0] = data.dwidth;
    l[1] = data.dheight;
    EWMH(data.root, _NET_DESKTOP_GEOMETRY,    XA_CARDINAL, 32, &l, 2);
    l[0] = 1;
    EWMH(data.root, _NET_NUMBER_OF_DESKTOPS,  XA_CARDINAL, 32, &l, 1);
#undef EWMH
    ewmh_area();
    ewmh_frame();
}

void
ewmh_list(void) {
    Win *w;
    List *l;
    Atom a[2];
    unsigned int i;

    a[0] = ewmh[_NET_CLIENT_LIST];
    a[1] = ewmh[_NET_CLIENT_LIST_STACKING];
    for(i = 0; i < LEN(a); i++) {
        XChangeProperty(data.dpy, data.root, a[i], XA_WINDOW, 32,
                        PropModeReplace, (unsigned char*)&data.empty, 1);
        if(win_first(data.tag, NORMAL, &l, &w)) {
            XDeleteProperty(data.dpy, data.root, a[i]);
            for(; l; win_next(data.tag, NORMAL, &l, &w))
                XChangeProperty(data.dpy, data.root, a[i], XA_WINDOW, 32,
                                PropModeAppend, (unsigned char*)&w->win, 1);
        }
    }
}

char*
ewmh_str(int i) {
    char *c[LASTEwmh] = {
        [_NET_SUPPORTED]                  = "_NET_SUPPORTED",
        [_NET_CLIENT_LIST]                = "_NET_CLIENT_LIST",
        [_NET_CLIENT_LIST_STACKING]       = "_NET_CLIENT_LIST_STACKING",
        [_NET_NUMBER_OF_DESKTOPS]         = "_NET_NUMBER_OF_DESKTOPS",
        [_NET_DESKTOP_GEOMETRY]           = "_NET_DESKTOP_GEOMETRY",
        [_NET_DESKTOP_VIEWPORT]           = "_NET_DESKTOP_VIEWPORT",
        [_NET_CURRENT_DESKTOP]            = "_NET_CURRENT_DESKTOP",
        [_NET_DESKTOP_NAMES]              = "_NET_DESKTOP_NAMES",
        [_NET_ACTIVE_WINDOW]              = "_NET_ACTIVE_WINDOW",
        [_NET_WORKAREA]                   = "_NET_WORKAREA",
        [_NET_SUPPORTING_WM_CHECK]        = "_NET_SUPPORTING_WM_CHECK",
        [_NET_CLOSE_WINDOW]               = "_NET_CLOSE_WINDOW",
        [_NET_WM_NAME]                    = "_NET_WM_NAME",
        [_NET_WM_DESKTOP]                 = "_NET_WM_DESKTOP",
        [_NET_WM_WINDOW_TYPE]             = "_NET_WM_WINDOW_TYPE",
        [_NET_WM_WINDOW_TYPE_DESKTOP]     = "_NET_WM_WINDOW_TYPE_DESKTOP",
        [_NET_WM_WINDOW_TYPE_DOCK]        = "_NET_WM_WINDOW_TYPE_DOCK",
        [_NET_WM_WINDOW_TYPE_TOOLBAR]     = "_NET_WM_WINDOW_TYPE_TOOLBAR",
        [_NET_WM_WINDOW_TYPE_MENU]        = "_NET_WM_WINDOW_TYPE_MENU",
        [_NET_WM_WINDOW_TYPE_UTILITY]     = "_NET_WM_WINDOW_TYPE_UTILITY",
        [_NET_WM_WINDOW_TYPE_SPLASH]      = "_NET_WM_WINDOW_TYPE_SPLASH",
        [_NET_WM_WINDOW_TYPE_DIALOG]      = "_NET_WM_WINDOW_TYPE_DIALOG",
        [_NET_WM_WINDOW_TYPE_NORMAL]      = "_NET_WM_WINDOW_TYPE_NORMAL",
        [_NET_WM_STATE]                   = "_NET_WM_STATE",
        [_NET_WM_STATE_MODAL]             = "_NET_WM_STATE_MODAL",
        [_NET_WM_STATE_STICKY]            = "_NET_WM_STATE_STICKY",
        [_NET_WM_STATE_MAXIMIZED_VERT]    = "_NET_WM_STATE_MAXIMIZED_VERT",
        [_NET_WM_STATE_MAXIMIZED_HORZ]    = "_NET_WM_STATE_MAXIMIZED_HORZ",
        [_NET_WM_STATE_SKIP_TASKBAR]      = "_NET_WM_STATE_SKIP_TASKBAR",
        [_NET_WM_STATE_SKIP_PAGER]        = "_NET_WM_STATE_SKIP_PAGER",
        [_NET_WM_STATE_HIDDEN]            = "_NET_WM_STATE_HIDDEN",
        [_NET_WM_STATE_FULLSCREEN]        = "_NET_WM_STATE_FULLSCREEN",
        [_NET_WM_STATE_ABOVE]             = "_NET_WM_STATE_ABOVE",
        [_NET_WM_STATE_BELOW]             = "_NET_WM_STATE_BELOW",
        [_NET_WM_STATE_DEMANDS_ATTENTION] = "_NET_WM_STATE_DEMANDS_ATTENTION",
        [_NET_WM_STRUT]                   = "_NET_WM_STRUT",
        [_NET_WM_STRUT_PARTIAL]           = "_NET_WM_STRUT_PARTIAL",
        [_NET_WM_PID]                     = "_NET_WM_PID",
        [_NET_FRAME_EXTENTS]              = "_NET_FRAME_EXTENTS"
    };
    return c[i];
}
