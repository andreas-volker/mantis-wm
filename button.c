enum { CLOSE, FOCUS, MOVE, RESIZE, TOGGLE };

char*
button_add(char *mask, char *argstr) {
    Button *b;
    char *c;
    unsigned int i, mod, button;
    struct { char *c; int i; } nums[] = {
        { "CLOSE", CLOSE }, { "RESIZE", RESIZE },
        { "FOCUS", FOCUS }, { "TOGGLE", TOGGLE }, { "MOVE", MOVE }};

    rc_mask(mask, &mod, &c);
    button = !strstr(c, "Button") ? 0 : atoi(c + strlen("Button"));
    free(c);
    if(!button)
        return "Not a valid button.";
    for(i = 0; i < LEN(nums); i++)
        if(!strcmp(nums[i].c, argstr))
            break;
    if(i == LEN(nums))
        return "Not a valid argument.";
    b = emalloc(sizeof(Button));
    b->mod = mod;
    b->button = button;
    b->arg = nums[i].i;
    data.buttons = list_insert(data.buttons, b, -1);
    return NULL;
}

void
button_free(void) {
    List *l;
    Button *b;

    for(l = list_first(data.buttons); l;) {
        b = (Button*)l->ptr;
        l = l->next;
        data.buttons = list_remove(data.buttons, b);
        free(b);
    }
}

void
button_func(Win *w, unsigned int i) {
    if(w && i == FOCUS)
        win_focus(w->win);
    else if(w && i == CLOSE)
        win_close(w->win);
    if(!w || i == FOCUS || i == CLOSE ||
       XGrabPointer(data.dpy, w->win, False, ButtonPressMask|ButtonReleaseMask|
                    PointerMotionMask, GrabModeAsync, GrabModeAsync, None,
                    None, CurrentTime) != GrabSuccess)
        return;
    if(i == TOGGLE)
        win_toggle(w);
    else if(w->type & NORMAL) {
        w->newtype = (w->type & ~NORMAL) | FLOAT;
        win_updatetype(w);
        layout_arrange();
    }
    if(w->win != data.current)
        win_focus(w->win);
    else
        win_stackreorder();
    if(i == MOVE || i == RESIZE)
        moveresizemouse(w, i);
    XUngrabPointer(data.dpy, CurrentTime);
    win_resize(w);
}

void
button_grab(Window win) {
    List *l;
    Button *b;
    unsigned int i, x = LockMask, y = Mod2Mask, z = Mod3Mask;
    unsigned int lock[] = { 0, x, y, z, x|y, x|z, y|z, x|y|z };

    XUngrabButton(data.dpy, AnyButton, AnyModifier, win);
    for(l = list_first(data.buttons); l; l = l->next) {
        b = (Button*)l->ptr;
        for(i = 0; i < LEN(lock); i++)
            XGrabButton(data.dpy, b->button, b->mod|lock[i], win, False,
                        ButtonPressMask|ButtonReleaseMask, GrabModeAsync,
                        GrabModeSync, None, None);
    }
}

void
moveresizemouse(Win *w, unsigned int i) {
    int x, y;
    XEvent e;
    XWindowAttributes a;

    x = y = -1;
    XGetWindowAttributes(data.dpy, w->win, &a);
    do{ XMaskEvent(data.dpy, ButtonPressMask|ButtonReleaseMask|
                   PointerMotionMask|SubstructureRedirectMask, &e);
        if(e.type == MotionNotify) {
            if(x == -1)
                x = i == MOVE ? e.xmotion.x : a.width - e.xmotion.x;
            if(y == -1)
                y = i == MOVE ? e.xmotion.y : a.height - e.xmotion.y;
            if(i == MOVE) {
                w->x = e.xmotion.x_root - x;
                w->y = e.xmotion.y_root - y;
                XMoveWindow(data.dpy, w->win, w->x, w->y);
            } else if(i == RESIZE) {
                w->w = MAX(1, e.xmotion.x + x);
                w->h = MAX(1, e.xmotion.y + y);
                icccm_size(w);
                XResizeWindow(data.dpy, w->win, w->w, w->h);
            }
        } else if(e.type == ConfigureRequest)
            event[e.type](&e);
    } while(e.type != ButtonRelease);
}
