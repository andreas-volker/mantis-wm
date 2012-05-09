char*
layout_add(char *name, char *str, int ntok) {
    List *l;
    Layout *lo;
    Laydata *ld;
    int i, j, a[6];
    char *t[ntok], *c[ntok];

    if(name) {
        free(var.layout);
        var.layout = estrdup(name);
        lo = emalloc(sizeof(Layout));
        lo->name = estrdup(name);
        lo->data = NULL;
        data.layouts = list_insert(data.layouts, lo, -1);
    } else if(str && var.layout) {
        c[0] = (t[0] = strtok(str, ";"));
        for(i = 1; (t[i] = strtok(NULL, ";")); i++) {
            for(; t[i] && !isdigit(t[i][0]) && t[i][0] != '-'; t[i]++);
            c[i] = t[i];
        }
        c[i] = NULL;
        for(i = 0, j = 0, l = NULL; c[i]; i++) {
            memset(&a, 0, sizeof(a));
            if((j = sscanf(c[i], "%d,%d,%d,%d,%d,%d", &a[0], &a[1], &a[2],
                           &a[3], &a[4], &a[5])) < 4)
                continue;
            ld = emalloc(sizeof(Laydata));
            ld->x = a[0];
            ld->y = a[1];
            ld->width = MAX(1, a[2]);
            ld->height = MAX(1, a[3]);
            ld->col = MAX(0, a[4]);
            ld->row = MAX(0, a[5]);
            l = list_insert(l, ld, -1);
        }
        if(!l)
            return "Nothing to insert into layout";
        if(!(lo = layout_get(var.layout)))
            return "Layout variable changed while parsing and do not exist.";
        lo->tile = j == 6;
        lo->data = list_insert(lo->data, l, -1);
    }
    return NULL;
}

void
layout_arrange(void) {
    Win *w;
    List *l, *ll;
    Layout *lo;
    Laydata *ld;
    unsigned int i, j, len;

    if(!(lo = layout_get(var.layout)))
        return;
    for(l = list_first(data.wins); l; l = l->next) {
        w = (Win*)l->ptr;
        if(!(w->tag & data.tag))
            XMoveWindow(data.dpy, w->win, w->x, (data.height * 2));
        else if(w->type & FULL)
            return;
        else if(w->type & FLOAT)
            XMoveWindow(data.dpy, w->win, w->x, w->y);
    }
    j = win_len(data.tag, NORMAL) - 1;
    if(win_first(data.tag, NORMAL, &l, &w))
        for(i = 0; w; win_next(data.tag, NORMAL, &l, &w), i++) {
            if(!(l = list_nth(lo->data, i)))
                l = list_last(lo->data);
            len = list_len(l);
            j = lo->tile ? MIN(len, j) : j;
            l = (List*)l->ptr;
            if(!(ll = list_nth(l, (j - i))))
                ll = list_last(l);
            ld = (Laydata*)ll->ptr;
            w->w = ((ld->width * data.width) / 100);
            w->h = ((ld->height * data.height) / 100);
            w->x = ((ld->x * data.width) / 100);
            w->x += var.gap_left + ROUND(var.gap_win / 2) - var.border_width;
            w->y = ((ld->y * data.height) / 100);
            w->y += var.gap_top + ROUND(var.gap_win / 2) - var.border_width;
            if(!lo->tile || i != len - 1) {
                w->w -= var.gap_win;
                w->h -= var.gap_win;
                win_resize(w);
            } else {
                tile(w, w->x, w->y, w->w, w->h, ld->col, ld->row);
                break;
            }
        }
}

void
layout_free(void) {
    List *l, *ll, *lll;
    Layout *lo;
    Laydata *ld;

    for(l = list_first(data.layouts); l;) {
        lo = (Layout*)l->ptr;
        l = l->next;
        data.layouts = list_remove(data.layouts, lo);
        for(ll = list_first(lo->data); ll;) {
            lll = (List*)ll->ptr;
            ll = list_remove(ll, lll);
            for(lll = list_first(lll); lll;) {
                ld = (Laydata*)lll->ptr;
                lll = list_remove(lll, ld);
                free(ld);
            }
        }
        free(lo->name);
        free(lo);
    }
}

Layout*
layout_get(char *str) {
    List *l;
    Layout *lo;

    for(l = list_first(data.layouts); l; l = l->next) {
        lo = (Layout*)l->ptr;
        if(!strcmp(lo->name, str))
            return lo;
    }
    return NULL;
}

void
layout_resize(int winc, int hinc) {
    Win *w;
    List *l, *ll;
    Layout *lo;
    Laydata *ld;
    int x, y, width, height;
    unsigned int i, j;

    if(!data.current || !(lo = layout_get(var.layout)))
        return;
    i = win_index(data.tag, NORMAL, data.current);
    j = win_len(data.tag, NORMAL) - 1;
    x = y = width = height = 0;
    if(win_first(data.tag, NORMAL, &l, &w))
        for(;w;) {
            if(!(l = list_nth(lo->data, i)))
                l = list_last(lo->data);
            j = lo->tile ? MIN(list_len(l), j) : j;
            l = (List*)l->ptr;
            if(!(ll = list_nth(l, (j - i))))
                ll = list_last(l);
            ld = (Laydata*)ll->ptr;
            if(!x && !y && !width && !height) {
                x = ld->x;
                y = ld->y;
                width = ld->width;
                height = ld->height;
                if(height >= 100)
                    hinc = 0;
                if(width >= 100)
                    winc = 0;
                i = 0;
                continue;
            }
            resize(x, width, &ld->x, (int*)&ld->width, winc);
            resize(y, height, &ld->y, (int*)&ld->height, hinc);
            win_next(data.tag, NORMAL, &l, &w);
            ++i;
        }
    layout_arrange();
}

void
resize(int x, int y, int *a, int *b, int i) {
    if((x + y >= 100 && *a == x) || *a == x + y) {
        *a += i;
        *b -= i;
    } else if((x + y >= 100 && *a + *b == x) ||
              (x + y < 100 && *a + *b == x + y))
        *b += i;
}

void
tile(Win *w, int x, int y, unsigned int width, unsigned int height,
     unsigned int col, unsigned int row) {
    Win *tmp;
    List *l;
    unsigned int len, xi, yi, i;

    if(!(tmp = w))
        return;
    for(len = 1; win_next(data.tag, NORMAL, &l, &tmp); len++);
    if(col && row)
        row = col = 0;
    if(!col) {
        if(row)
            col = ROUND(len / row); 
        else for(col = 0; col <= len; col++)
            if(col * col >= len)
                break;
    }
    if(!row)
        row = ROUND(len / col);
    for(i = 0, yi = 0; w; win_next(data.tag, NORMAL, &l, &w), i++) {
        if(!(xi = (i % col)) && i)
            ++yi;
        w->w = (width / col) - var.gap_win;
        w->h = (height / row) - var.gap_win;
        w->x = ((width / col) * xi) + x;
        w->y = ((height / row) * yi) + y;
        if((xi + 1) == col || (i + 1) == len)
            w->w += width - ((w->w + var.gap_win) * (xi + 1));
        if((yi + 1) == row || ((yi + 1) * col) >= len)
            w->h += height - ((w->h + var.gap_win) * (yi + 1));
        win_resize(w);
    }
}
