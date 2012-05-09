void
sink_add(Window win) {
    Sink *s;

    if(getsink(win))
        return;
    s = emalloc(sizeof(Sink));
    s->win = win;
    data.sinks = list_insert(data.sinks, s, -1);
}

void
sink_del(Window win) {
    Sink *s;
    List *l;

    if(!(s = getsink(win)))
        return;
    data.sinks = list_remove(data.sinks, s);
    free(s);
    win = (l = list_last(data.sinks)) && (s = (Sink*)l->ptr) ? s->win : None;
    data.lastsink = win;
}

void
sink_free(void) {
    Sink *s;
    List *l;

    for(l = list_first(data.sinks); l;) {
        s = (Sink*)l->ptr;
        l = l->next;
        data.sinks = list_remove(data.sinks, s);
        free(s);
    }
    data.lastsink = None;
}

Window
sink_last(void) {
    Sink *s;
    List *l;

    if(!(l = list_last(data.sinks)))
        return None;
    s = (Sink*)l->ptr;
    return s->win;
}

void
sink_update(void) {
    Sink *s;
    List *l;
    int i;
    Window win, *wins;

    if(!(win = sink_last()) || win == data.lastsink)
        return;
    XLowerWindow(data.dpy, win);
    data.lastsink = win;
    if((i = list_len(data.sinks)) < 2)
        return;
    wins = emalloc(sizeof(Window) * i);
    for(l = list_last(data.sinks), i = 0; l; l = l->prev, i++) {
        s = (Sink*)l->ptr;
        wins[i] = s->win;
    }
    XRestackWindows(data.dpy, wins, i);
    free(wins);
}

Sink*
getsink(Window win) {
    Sink *s;
    List *l;

    for(l = list_first(data.sinks); l; l = l->next) {
        s = (Sink*)l->ptr;
        if(s->win == win) {
            return s;
        }
    }
    return NULL;
}
