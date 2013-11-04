void
atom_free(void) {
    XDeleteProperty(data.dpy, data.root, data.atom[1]);
    XDeleteProperty(data.dpy, data.root, data.atom[2]);
}

void
atom_init(void) {
    data.atom[0] = XInternAtom(data.dpy, "UTF8_STRING", False);
    data.atom[1] = XInternAtom(data.dpy, "_MANTIS_TAG", False);
    data.atom[2] = XInternAtom(data.dpy, "_MANTIS_VIEW", False);
}

void
atom_update(void) {
    char c[BUFSIZ];
    unsigned int i, j, k;
    unsigned long l;

    atom_free();
    for(i = 0, j = 1; (1<<i) <= (int)data.tagmask + 1; i++) {
        if((1<<i) <= (int)data.tagmask) {
            l = 1<<i;
            sprintf(c, "%d", (data.tag & l) > 0);
        } else for(k = 0, j = 2, l = data.tag; k < i; k++)
            sprintf(c, "%s%d", c, (data.tag & (1<<k)) > 0);
        sprintf(c, "%s %d %d %d %d %d", c, win_index(l, ~0, data.current) + 1,
                win_len(l, ~0), win_len(l, NORMAL), win_len(l, FLOAT),
                win_len(l, URGENT));
        XChangeProperty(data.dpy, data.root, data.atom[j], data.atom[0], 8,
                        PropModeAppend, (unsigned char*)c, strlen(c) + 1);
        memset(&c, '\0', sizeof(c));
    }
}

