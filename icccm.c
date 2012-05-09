enum {
    WM_STATE,
    WM_PROTOCOLS,
    WM_TAKE_FOCUS,
    WM_DELETE_WINDOW,
    LASTIcccm
};

static Atom icccm[LASTIcccm];

void
icccm_init(void) {
    int i;

    for(i = 0; i < LASTIcccm; i++)
        icccm[i] = XInternAtom(data.dpy, icccm_str(i), False);
}

void
icccm_check(Window win, unsigned int *type) {
    Window t;
    XWMHints *h;

    *type &= TYPEMASK;
    if(!(*type & NORMAL))
        if(XGetTransientForHint(data.dpy, win, &t) && win_get(t))
            *type |= FLOAT;
    if((h = XGetWMHints(data.dpy, win))) {
        if(h->flags & XUrgencyHint)
            *type |= URGENT;
        if((h->flags & InputHint) && !h->input)
            *type = (*type & ~(INPUT|NORMAL)) | FLOAT;
        XFree(h);
    }
}

Bool
icccm_sendmsg(Window win, Atom a) {
    int i;
    Atom *atoms;
    Bool b;
    XEvent e;

    b = False;
    if(!win || !a)
        return b;
    if(XGetWMProtocols(data.dpy, win, &atoms, &i)) {
        while(--i >= 0 && !(b = atoms[i] == a));
        XFree(atoms);
    }
    if(b) {
        memset(&e, 0, sizeof(e));
        e.xclient.type = ClientMessage;
        e.xclient.display = data.dpy;
        e.xclient.window = win;
        e.xclient.message_type = icccm[WM_PROTOCOLS];
        e.xclient.format = 32;
        e.xclient.data.l[0] = a;
        e.xclient.data.l[1] = CurrentTime;
        XSendEvent(data.dpy, win, False, NoEventMask, &e);
    }
    return b;
}

void
icccm_size(Win *w) {
    int x, y;
    long dl;
    double width, height;
    XSizeHints s;

    if(!w || !XGetWMNormalHints(data.dpy, w->win, &s, &dl))
        return;
    if(w->type & NORMAL && s.flags & PMinSize && s.flags & PMaxSize &&
       s.max_width == s.min_width && s.max_height == s.min_height) {
        w->w = s.min_width;
        w->h = s.min_height;
        win_toggle(w);
    } else if(s.flags & PAspect) {
        x = s.max_aspect.x + s.min_aspect.x;
        y = s.max_aspect.y + s.min_aspect.y;
        width = (double)x / y;
        height = (double)y / x;
        if(width < ((double)w->w / w->h))
            w->w = ROUND(w->h * width);
        else if(height < ((double)w->h / w->w))
            w->h = ROUND(w->w * height);
    }
}

char*
icccm_str(int i) {
    char *c[LASTIcccm] = {
        [WM_STATE]         = "WM_STATE",
        [WM_PROTOCOLS]     = "WM_PROTOCOLS",
        [WM_TAKE_FOCUS]    = "WM_TAKE_FOCUS",
        [WM_DELETE_WINDOW] = "WM_DELETE_WINDOW",
    };

    return c[i];
}
