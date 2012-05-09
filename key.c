enum { SELECT, CHANGE, CURRENT, ALL, ALLELSE };

char*
key_add(char *mask, char *func, char *(*karg)[2], char (*ktype)[2]) {
    Key *k;
    Arg a[2];
    char *key;
    KeySym ks;
    unsigned int mod, i, ki, ni[2];
    struct { char *c; int i; } nums[] = {
        { "NORMAL",  NORMAL  }, { "FLOAT",   FLOAT   }, { "INPUT",   INPUT   },
        { "URGENT",  URGENT  }, { "SELECT",  SELECT  }, { "CHANGE",  CHANGE  },
        { "CURRENT", CURRENT }, { "ALL",     ALL     }, { "ALLELSE", ALLELSE },
        { "SIGKILL", SIGKILL }, { "SIGTERM", SIGTERM }, { "SIGCONT", SIGCONT },
        { "SIGSTOP", SIGSTOP }};
    struct { char type[2], *id; void (*func)(Win*, Arg*(*)[2]); } keys[] = {
        { "n-", "close",   &keyclose   }, { "ni", "focus",   &keyfocus   },
        { "n-", "kill",    &keykill    }, { "c-", "macro",   &keymacro   },
        { "ii", "move",    &keymove    }, { "i-", "quit",    &keyquit    },
        { "ii", "resize",  &keyresize  }, { "--", "restart", &keyrestart },
        { "cc", "set",     &keyset     }, { "c-", "spawn",   &keyspawn   },
        { "i-", "switch",  &keyswitch  }, { "ni", "tag",     &keytag     },
        { "--", "toggle",  &keytoggle  }};

    memset(&a, 0, sizeof(a));
    memset(&ni, 0, sizeof(ni));
    rc_mask(mask, &mod, &key);
    ks = XStringToKeysym(key);
    free(key);
    if(ks == NoSymbol)
        return "Not a valid key.";
    for(ki = 0; ki < LEN(keys); ki++)
        if(!strcmp(keys[ki].id, func))
            break;
    if(ki == LEN(keys))
        return "Not a valid function";
    for(i = 0; i < LEN(a); i++) {
        if((*ktype)[i] != keys[ki].type[i])
            return "Not a valid argument.";
        else if((*ktype)[i] == 'n')
            for(ni[i] = 0; ni[i] < LEN(nums); ni[i]++)
                if(!strcmp(nums[ni[i]].c, (*karg)[i]))
                    break;
        if(ni[i] == LEN(nums))
            return "Not a valid argument.";
    }
    k = emalloc(sizeof(Key));
    k->id = estrdup(mask);
    k->mod = mod;
    k->key = ks;
    k->func = keys[ki].func;
    for(i = 0; i < LEN(a); i++) {
        switch((*ktype)[i]) {
        case 'n': a[i].i = nums[ni[i]].i; break;
        case 'i': a[i].i = atoi((*karg)[i]); break;
        case 'c': a[i].c = estrdup((*karg)[i]); break;
        }
        k->arg[i] = a[i];
    }
    data.keys = list_insert(data.keys, k, -1);
    return NULL;
}

void
key_free(void) {
    Key *k;
    List *l;

    for(l = list_first(data.keys); l;) {
        k = (Key*)l->ptr;
        l = l->next;
        data.keys = list_remove(data.keys, k);
        free(k->id);
        free(k->arg[0].c);
        free(k->arg[1].c);
        free(k);
    }
}

void
key_grab(void) {
    Key *k;
    List *l;
    KeyCode kc;
    unsigned int i, x = LockMask, y = Mod2Mask, z = Mod3Mask;
    unsigned int lock[] = { 0, x, y, z, x|y, x|z, y|z, x|y|z };

    XUngrabKey(data.dpy, AnyKey, AnyModifier, data.root);
    for(l = list_first(data.keys); l; l = l->next) {
        k = (Key*)l->ptr;
        if((kc = XKeysymToKeycode(data.dpy, k->key)))
            for(i = 0; i < LEN(lock); i++)
                XGrabKey(data.dpy, kc, k->mod|lock[i], data.root,
                         True, GrabModeAsync, GrabModeAsync);
    }
}

void
keyclose(Win *w, Arg *(*a)[2]) {
    List *l;
    Window win;

    if(!w)
        return;
    win = w->win;
    if((*a)[0]->i == CURRENT)
        win_close(win);
    else if(win_first(data.tag, NORMAL, &l, &w))
        for(; w; win_next(data.tag, NORMAL, &l, &w))
            if(((*a)[0]->i == ALLELSE && w->win != win) ||
               (*a)[0]->i == ALL)
                win_close(w->win);
}

void
keyfocus(Win *w, Arg *(*a)[2]) {
    Win *s;
    Window win;
    unsigned long l;

    win = w ? w->win : None;
    l = (*a)[0]->i == URGENT ? data.tagmask & ~0 : data.tag;
    if(!w && (*a)[0]->i != URGENT)
        return;
    s = NULL;
    win_neighbor(l, (*a)[0]->i, win, (*a)[1]->i, None, 0, 0, NULL, &s);
    if(!s)
        return;
    l = data.tag & s->tag ? data.tag : s->tag;
    if(l != data.tag) {
        data.tag = l;
        layout_arrange();
        ewmh_list();
        if(w && s->win == w->win)
            win_stackreorder();
    }
    win_focus(s->win);
}

void
keykill(Win *w, Arg *(*a)[2]) {
    int di;
    Atom da;
    unsigned long dl, *pid;

    pid = 0L;
    if(!w || XGetWindowProperty(data.dpy, w->win, ewmh[_NET_WM_PID], 0L, 1L,
                                False, XA_CARDINAL, &da, &di, &dl, &dl,
                                (unsigned char**)&pid) != Success)
        return;
    if(*pid)
        kill(*pid, (*a)[0]->i);
    XFree(pid);
}

void
keymacro(Win *w, Arg *(*a)[2]) {
    Key *k;
    Arg *arg[2];
    List *l;
    size_t len;
    unsigned int i, j;
    (void)w;

    len = strlen((*a)[0]->c);
    char buf[len];
    strcpy(buf, (*a)[0]->c);
    for(i = 0, j = 2; i < len; i++)
        if(!isalpha(buf[i]) && buf[i + 1] == '<')
            j++;
    char *t[j];
    t[0] = strtok(buf, " <>");
    for(i = 1; (t[i] = strtok(NULL, " <>")); i++);
    for(i = 0; t[i]; i++)
        for(l = list_first(data.keys); l; l = l->next) {
            k = (Key*)l->ptr;
            if(!strcmp(t[i], k->id)) {
                arg[0] = &k->arg[0];
                arg[1] = &k->arg[1];
                k->func(win_get(data.current), &arg);
                break;
            }
        }
}

void
keymove(Win *w, Arg *(*a)[2]) {
    if(!w || !(w->type & FLOAT))
        return;
    w->x += (data.width / 100) * (*a)[0]->i;
    w->y += (data.height / 100) * (*a)[1]->i;
    win_resize(w);
}

void
keyquit(Win *w, Arg *(*a)[2]) {
    (void)w;

    data.ret = (*a)[0]->i ? EXIT_FAILURE : EXIT_SUCCESS;
    data.run = False;
}

void
keyresize(Win *w, Arg *(*a)[2]) {
    if(w->type & FLOAT) {
        w->w += (data.width / 100) * (*a)[0]->i;
        w->h += (data.height / 100) * (*a)[1]->i;
        win_resize(w);
    } else if(w->type & NORMAL)
        layout_resize((*a)[0]->i, (*a)[1]->i);
}

void
keyrestart(Win *w, Arg *(*a)[2]) {
    List *l;
    (void)a;

    button_free();
    key_free();
    layout_free();
    rc_init();
    key_grab();
    for(l = list_first(data.wins); l; l = l->next) {
        w = (Win*)l->ptr;
        button_grab(w->win);
    }
    ewmh_area();
    ewmh_frame();
    win_updateborder();
    layout_arrange();
}

void
keyset(Win *w, Arg *(*a)[2]) {
    size_t len;
    char *c;
    unsigned int i, j;
    (void)w;

    len = strlen((*a)[1]->c);
    char buf[len];
    strcpy(buf, (*a)[1]->c);
    for(i = 0, j = 2; i < len; i++)
        if(buf[i] == ' ')
            ++j;
    char *t[j], *v[j];
    v[0] = (t[0] = strtok(buf, " "));
    for(i = 1; (v[i] = (t[i] = strtok(NULL, " "))); i++);
    if((c = var_set((*a)[0]->c, v)))
        fprintf(stderr, PROGRAM": %s\n", c);
    ewmh_area();
    ewmh_frame();
    layout_arrange();
    win_updateborder();
}

void
keyspawn(Win *w, Arg *(*a)[2]) {
    size_t len;
    unsigned int i, j;
    (void)w;

    len = strlen((*a)[0]->c);
    char buf[len];
    strcpy(buf, (*a)[0]->c);
    for(i = 0, j = 2; i < len; i++)
        if(buf[i] == ' ')
            ++j;
    char *t[j], *v[j];
    v[0] = (t[0] = strtok(buf, " "));
    for(i = 1; (v[i] = (t[i] = strtok(NULL, " "))); i++);
	if(fork() == 0) {
		setsid();
		execvp(v[0], v);
		perror(v[0]);
		exit(EXIT_SUCCESS);
    }
}

void
keyswitch(Win *w, Arg *(*a)[2]) {
    int i;

    if(!w || !(w->type & NORMAL))
        return;
    win_neighbor(data.tag, NORMAL, w->win, (*a)[0]->i, None, 0, 0, &i, NULL);
    data.wins = list_remove(data.wins, w);
    data.wins = list_insert(data.wins, w, i);
    layout_arrange();
    ewmh_list();
    win_updateinfo();
}

void
keytag(Win *w, Arg *(*a)[2]) {
    unsigned long l;

    l = !(*a)[1]->i ? ~0 & data.tagmask : (1<<((*a)[1]->i - 1)) & data.tagmask;
    if((*a)[0]->i == SELECT)
        data.tag = data.keypress ? data.tag|l : l;
    else if((*a)[0]->i == CHANGE) {
        if(!w)
            return;
        w->tag = data.keypress ? w->tag|l : l;
    }
    layout_arrange();
    ewmh_list();
    win_focus(data.current);
    win_stackreorder();
    win_updateborder();
    data.keypress = True;
    win_updateinfo();
}

void
keytoggle(Win *w, Arg *(*a)[2]) {
    (void)a;

    win_toggle(w);
}
