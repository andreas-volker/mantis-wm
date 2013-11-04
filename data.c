void
data_cmdfree(void) {
    unsigned int i;

    if(!data.cmd)
        return;
    for(i = 0; data.cmd[i]; i++)
        free(data.cmd[i]);
    free(data.cmd);
}

void
data_listfree(void) {
    Win *w;
    List *l;

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
}
