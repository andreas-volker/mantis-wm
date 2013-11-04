void
win_close(Window win) {
    if(!icccm_sendmsg(win, icccm[WM_DELETE_WINDOW])) {
        XSetCloseDownMode(data.dpy, DestroyAll);
        XKillClient(data.dpy, win);
    }
}

void
win_focus(Window win) {
    Win *w;
    List *l;

    if(data.current && (!(w = win_get(data.current)) || !(w->tag & data.tag)))
        data.current = None;
    if(!(w = win_get(win)) && !(win = data.current))
        win_first(data.tag, INPUT, &l, &w);
    if(w && (!(w->tag & data.tag) || (!(w->type & INPUT) && !data.current)))
        if(!win_next(data.tag, INPUT, &l, &w))
            win_last(data.tag, INPUT, &l, &w);
    if(w && w->tag & data.tag && w->type & INPUT)
        win = w->win;
    else if(w && w->tag & data.tag && !(w->type & INPUT))
        win = data.current;
    else
        win = None;
    if(data.current && win && data.current == win)
        return;
    if(!win) {
        XSetInputFocus(data.dpy, PointerRoot, RevertToNone, CurrentTime);
        XChangeProperty(data.dpy, data.root, ewmh[_NET_ACTIVE_WINDOW],
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char*)&data.empty, 1);
        atom_update();
        return;
    }
    data.prev = data.current;
    data.current = win;
    win_stackreorder();
    win_updateborder();
    icccm_sendmsg(win, icccm[WM_TAKE_FOCUS]);
    XSetInputFocus(data.dpy, win, RevertToPointerRoot, CurrentTime);
    XChangeProperty(data.dpy, data.root, ewmh[_NET_ACTIVE_WINDOW], XA_WINDOW,
                    32, PropModeReplace, (unsigned char*)&win, 1);
    atom_update();
}

void
win_manage(Window win) {
    Win *w;
    int i;
    XWindowAttributes a;

    if(win_get(win))
        return;
    w = emalloc(sizeof(Win));
    w->win = win;
    w->tag = data.tag;
    w->type = w->newtype = 0;
    XGetWindowAttributes(data.dpy, w->win, &a);
    w->w = a.width;
    w->h = a.height;
    w->x = a.x ? a.x : (data.width / 2) - ((int)w->w / 2);
    w->y = a.y ? a.y : (data.height / 2) - ((int)w->h / 2);
    win_neighbor(data.tag, NORMAL, data.current, var.newwin, None, 0, 1, &i,
                 NULL);
    i += var.newwin < 0 && i > 0 ? 1 : 0;
    data.wins = list_insert(data.wins, w, i);
    XSelectInput(data.dpy, w->win, EnterWindowMask|PropertyChangeMask);
    XSetWindowBorderWidth(data.dpy, w->win, var.border_width);
    button_grab(w->win);
    win_updatetype(w);
    if(w->type & NORMAL)
        data.stack = list_insert(data.stack, w, -1);
    ewmh_list();
}

void
win_map(Window win) {
    Win *w;

    if(win_get(win))
        return;
    win_manage(win);
    layout_arrange();
    XMapWindow(data.dpy, win);
    if(win_get(win)) {
        if((w = win_get(data.current)) && (w->type & FULL))
            XLowerWindow(data.dpy, win);
        else
            win_focus(win);
    }
}

void
win_resize(Win *w) {
    XEvent e;

    if(!w || w->type & FULL)
        return;
    icccm_size(w);
    memset(&e, 0, sizeof(e));
    e.xconfigure.type              = ConfigureNotify;
    e.xconfigure.display           = data.dpy;
    e.xconfigure.event             = w->win;
    e.xconfigure.window            = w->win;
    e.xconfigure.x                 = w->x;
    e.xconfigure.y                 = w->y;
    e.xconfigure.width             = w->w;
    e.xconfigure.height            = w->h;
    e.xconfigure.border_width      = var.border_width;
    e.xconfigure.above             = None;
    e.xconfigure.override_redirect = False;
    XSendEvent(data.dpy, w->win, False, StructureNotifyMask, &e);
    XMoveResizeWindow(data.dpy, w->win, w->x, w->y, w->w, w->h);
}

void
win_stackreorder(void) {
    Win *w;
    List *l;
    XWindowChanges c;

    if(!(w = win_get(data.current)))
        return;
    sink_update();
    if(w->type & (FULL|FLOAT)) {
        XRaiseWindow(data.dpy, w->win);
        return;
    }
    if(list_index(data.stack, w) > 0) {
        data.stack = list_remove(data.stack, w);
        data.stack = list_insert(data.stack, w, 0);
    }
    l = list_last(data.stack);
    w = (Win*)l->ptr;
    c.stack_mode = Above;
    if((c.sibling = sink_last())) {
        XConfigureWindow(data.dpy, w->win, CWStackMode|CWSibling, &c);
    } else
        XLowerWindow(data.dpy, w->win);
    for(; l && (c.sibling = w->win) && (l = l->prev) && (w = (Win*)l->ptr);)
        XConfigureWindow(data.dpy, w->win, CWStackMode|CWSibling, &c);
}

void
win_toggle(Win *w) {
    if(!w)
        return;
    if(w->type & FLOAT)
        w->newtype = (w->type & ~FLOAT) | NORMAL;
    else if(w->type & NORMAL) {
        w->x = (data.width / 2) - (w->w / 2);
        w->y = (data.height / 2) - (w->h / 2);
        w->newtype = (w->type & ~NORMAL) | FLOAT;
    }
    win_updatetype(w);
}

void
win_unmanage(Win *w) {
    Win *tmp;
    Window win;

    if(!w)
        return;
    XSelectInput(data.dpy, w->win, NoEventMask);
    if(data.current != w->win)
        win = data.current;
    else {
        data.current = None;
        if(w->type & (FLOAT|FULL) && win_get(data.prev))
            win = data.prev;
        else {
            tmp = NULL;
            win_neighbor(data.tag, NORMAL, w->win, var.focus, w->win, 0, 0,
                         NULL, &tmp);
            win = tmp ? tmp->win : None;
        }
    }
    data.wins = list_remove(data.wins, w);
    if(w->type & NORMAL)
        data.stack = list_remove(data.stack, w);
    free(w);
    layout_arrange();
    ewmh_list();
    win_focus(win);
}

void
win_updateborder(void) {
    Win *w;
    List *l;
    int i;

    if(win_first(data.tag, INPUT, &l, &w))
        for(; l; win_next(data.tag, INPUT, &l, &w)) {
            i = w->type & URGENT ? 2 : (w->win == data.current ? 0 : 1);
            XSetWindowBorder(data.dpy, w->win, data.border[i]);
        }
}

void
win_updatetype(Win *w) {
    unsigned int old;

    if(!w)
        return;
    if(!w->newtype) {
        w->newtype = (w->type & (NORMAL|FLOAT) & ~(SINK|URGENT|FULL)) | INPUT;
        ewmh_check(w->win, &w->newtype);
        icccm_check(w->win, &w->newtype);
        w->newtype = w->newtype & FLOAT ?
                     w->newtype & ~NORMAL : w->newtype|NORMAL;
    }
    if(w->newtype & SINK) {
        XSetWindowBorderWidth(data.dpy, w->win, 0);
        sink_add(w->win);
        win_unmanage(w);
        return;
    }
    old = w->type;
    w->type = w->newtype;
    w->newtype = 0;
    if((old & URGENT) != (w->type & URGENT))
        win_updateborder();
    if((!old || old & INPUT) && !(w->type & INPUT))
        XSetWindowBorderWidth(data.dpy, w->win, 1);
    if(!(old & FULL) && w->type & FULL) {
        XSetWindowBorderWidth(data.dpy, w->win, 0);
        XMoveResizeWindow(data.dpy, w->win, 0, 0, data.dwidth, data.dheight);
        XRaiseWindow(data.dpy, w->win);
        return;
    } else if(old & FULL && !(w->type & FULL))
        XSetWindowBorderWidth(data.dpy, w->win, var.border_width);
    if(old & NORMAL && w->type & FLOAT)
        data.stack = list_remove(data.stack, w);
    else if(old & FLOAT && w->type & NORMAL)
        data.stack = list_insert(data.stack, w, -1);
    if(old & (NORMAL|FULL) && w->type & FLOAT)
        win_resize(w);
    if((old & (NORMAL|FLOAT)) != (w->type & (NORMAL|FLOAT)))
        ewmh_list();
    if(old != w->type) {
        win_stackreorder();
        layout_arrange();
    }
    atom_update();
}

Bool
win_first(unsigned long tag, unsigned int type, List **l, Win **w) {
    tag &= data.tagmask;
    type &= TYPEMASK;
    for(*l = list_first(data.wins); *l; *l = (*l)->next) {
        *w = (Win*)(*l)->ptr;
        if((*w)->tag & tag && (*w)->type & type)
            return True;
    }
    *w = NULL;
    return False;
}

Win*
win_get(Window win) {
    Win *w;
    List *l;

    for(l = list_first(data.wins); l; l = l->next) {
        w = (Win*)l->ptr;
        if(w->win == win)
            return w;
    }
    return NULL;
}

int
win_index(unsigned long tag, unsigned int type, Window win) {
    Win *w;
    List *l;
    int i;

    if(win_first(tag, type, &l, &w))
        for(i = 0; l; win_next(tag, type, &l, &w), i++)
            if(w->win == win)
                return i;
    return -1;
}

Bool
win_last(unsigned long tag, unsigned int type, List **l, Win **w) {
    tag &= data.tagmask;
    type &= TYPEMASK;
    for((*l) = list_last(data.wins); *l; *l = (*l)->prev) {
        *w = (Win*)(*l)->ptr;
        if((*w)->tag & tag && (*w)->type & type)
            return True;
    }
    *w = NULL;
    return False;
}

unsigned int
win_len(unsigned long tag, unsigned int type) {
    Win *w;
    List *l;
    unsigned int i;

    for(win_first(tag, type, &l, &w), i = 0; l; win_next(tag, type, &l, &w))
        ++i;
    return i;
}

void
win_neighbor(unsigned long tag, unsigned int type, Window win, int inc,
             Window sibling, int offset, int lmt, int *iret, Win **wret) {
    Win *w;
    List *l;
    Bool b;
    int i, len, remain;

    remain = 0;
    len = (int)win_len(tag, type) - 1;
    if((i = win_index(tag, type, win)) < 0) {
        win_first(tag, type, &l, &w);
        if(wret)
            *wret = w;
        if(iret)
            *iret = 0;
        return;
    }
    i = !inc ? 0 : i + inc;
    i = CLAMP(i, 0 + offset, len + lmt);
    if((b = i > len)) {
        remain = i - len;
        i = len;
    }
    win_nth(tag, type, &l, &w, i);
    if(sibling && w && w->win == sibling) {
        if(inc < 0 && !win_prev(tag, type, &l, &w) && (w = win_get(win)))
            win_next(tag, type, &l, &w);
        else if(inc > 0 && !win_next(tag, type, &l, &w) && (w = win_get(win)))
            win_prev(tag, type, &l, &w);
        if(w && w->win == sibling && win_first(tag, type, &l, &w))
            if(w->win == sibling && !win_next(tag, type, &l, &w))
                return;
        if(!w || w->win == sibling)
            return;
    }
    len = (int)list_len(data.wins) - 1;
    i = CLAMP(list_index(data.wins, w), 0, len);
    if(iret)
        *iret = b ? len + remain : i;
    if(wret) {
        l = list_nth(data.wins, i);
        *wret = (Win*)l->ptr;
    }
}

Bool
win_next(unsigned long tag, unsigned int type, List **l, Win **w) {
    tag &= data.tagmask;
    type &= TYPEMASK;
    if((*l = list_find(data.wins, *w)))
        for(*l = (*l)->next; *l; *l = (*l)->next) {
            *w = (Win*)(*l)->ptr;
            if((*w)->tag & tag && (*w)->type & type)
                return True;
        }
    *w = NULL;
    return False;
}

Bool
win_nth(unsigned long tag, unsigned int type, List **l, Win **w,
        unsigned int index) {
    unsigned int i;

    if(win_first(tag, type, l, w))
        for(i = 0; *l; win_next(tag, type, l, w), i++)
            if(i == index)
                return True;
    *w = NULL;
    return False;
}

Bool
win_prev(unsigned long tag, unsigned int type, List **l, Win **w) {
    tag &= data.tagmask;
    type &= TYPEMASK;
    if((*l = list_find(data.wins, *w)))
        for(*l = (*l)->prev; *l; *l = (*l)->prev) {
            *w = (Win*)(*l)->ptr;
            if((*w)->tag & tag && (*w)->type & type)
                return True;
        }
    *w = NULL;
    return False;
}
