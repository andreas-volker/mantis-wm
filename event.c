static void (*event[LASTEvent])(XEvent*) = {
    [ButtonPress]       = buttonpress,
    [ClientMessage]     = clientmessage,
    [ConfigureRequest]  = configurerequest,
    [EnterNotify]       = enternotify,
    [KeyPress]          = keypress,
    [KeyRelease]        = keyrelease,
    [MapNotify]         = mapnotify,
    [MappingNotify]     = mappingnotify,
    [MapRequest]        = maprequest,
    [PropertyNotify]    = propertynotify,
    [DestroyNotify]     = unmapdestroynotify,
    [UnmapNotify]       = unmapdestroynotify
};

void
buttonpress(XEvent *e) {
    Win *w;
    List *l;
    Button *b;

    for(l = list_first(data.buttons); l; l = l->next) {
        b = (Button*)l->ptr;
        if(b->button == e->xbutton.button &&
           b->mod == (e->xbutton.state & MOD & ~LOCK)) {
            if((w = win_get(e->xbutton.window)) && w->type & FULL)
                return;
            button_func(w, b->arg);
            break;
        }
    }
}

void
clientmessage(XEvent *e) {
    Win *w;
    unsigned int type;

    if(e->xclient.message_type == ewmh[_NET_ACTIVE_WINDOW])
        win_focus(e->xclient.window);
    else if(e->xclient.message_type == ewmh[_NET_CLOSE_WINDOW])
        win_close(e->xclient.window);
    else if(e->xclient.message_type == ewmh[_NET_WM_STATE]) {
        ewmh_change(e->xclient.window, e->xclient.message_type,
                    e->xclient.data.l[1], e->xclient.data.l[0]);
        ewmh_change(e->xclient.window, e->xclient.message_type,
                    e->xclient.data.l[2], e->xclient.data.l[0]);
        if((w = win_get(e->xclient.window))) {
            win_updatetype(w);
            return;
        }
        type = 0;
        ewmh_check(e->xclient.window, &type);
        if(!(type & SINK)) {
            sink_del(e->xclient.window);
            win_manage(e->xclient.window);
        }
    }
}

void
configurerequest(XEvent *e) {
    Win *w;
    XConfigureRequestEvent *x = &e->xconfigurerequest;
    XWindowChanges c = { x->x, x->y, x->width, x->height, 0, 0L, 0 };

    if(!(x->value_mask & (CWBorderWidth|CWSibling|CWStackMode)) &&
       (!(w = win_get(x->window)) || w->type & FLOAT)) {
        XConfigureWindow(data.dpy, x->window, x->value_mask, &c);
        if(w) {
            w->x = c.x;
            w->y = c.y;
            w->w = c.width;
            w->h = c.height;
        }
    }
}

void
enternotify(XEvent *e) {
    Win *w;

    if(var.follow_mouse && e->xcrossing.state == var.follow_mod &&
       e->xcrossing.mode == NotifyNormal &&
       e->xcrossing.detail != NotifyInferior &&
       (w = win_get(e->xcrossing.window)) && w->type & INPUT) {
        win_focus(w->win);
    }
}

void
keypress(XEvent *e) {
    Win *w;
    Key *k;
    Arg *a[2];
    List *l;
    KeySym ks;

    ks = XkbKeycodeToKeysym(data.dpy, e->xkey.keycode, 0, 0);
    for(l = list_first(data.keys); l; l = l->next) {
        k = (Key*)l->ptr;
        if(k->key == ks && k->func &&
           k->mod == (e->xkey.state & ~LOCK & MOD)) {
            if((w = win_get(data.current)) && w->type & FULL &&
               k->func != keyquit && k->func != keyclose && k->func != keykill)
                return;
            a[0] = &k->arg[0];
            a[1] = &k->arg[1];
            k->func(w, &a);
            break;
        }
    }
}

void
keyrelease(XEvent *e) {
    (void)e;

    data.keypress = False;
}

void
mapnotify(XEvent *e) {
    if(e->xmap.override_redirect)
        sink_add(e->xmap.window);
}

void
mappingnotify(XEvent *e) {
    Win *w;
    List *l;

    XRefreshKeyboardMapping(&e->xmapping);
    if(e->xmapping.request != MappingPointer)
        key_grab();
    else for(l = list_first(data.wins); l; l = l->next) {
        w = (Win*)l->ptr;
        button_grab(w->win);
    }
}

void
maprequest(XEvent *e) {
    Win *w;
    Window win;

    win = e->xmaprequest.window;
    if(!win_get(win)) {
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
}

void
propertynotify(XEvent *e) {
    Win *w;

    if(!(w = win_get(e->xproperty.window)))
        return;
    else if(e->xproperty.atom == XA_WM_NORMAL_HINTS)
        win_resize(w);
    else if(e->xproperty.atom == XA_WM_HINTS ||
              e->xproperty.atom == XA_WM_TRANSIENT_FOR ||
              e->xproperty.atom == ewmh[_NET_WM_STATE] ||
              e->xproperty.atom == ewmh[_NET_WM_WINDOW_TYPE])
        win_updatetype(w);
}

void
unmapdestroynotify(XEvent *e) {
    Win *w;
    Window win;

    win = e->type == DestroyNotify ? e->xdestroywindow.window :
          (e->type == UnmapNotify ? e->xunmap.window : None);
    if((w = win_get(win)))
        win_unmanage(w);
    else if(win)
        sink_del(win);
}
