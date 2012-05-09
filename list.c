List*
list_find(List *l, void *ptr) {
    for(l = list_first(l); l; l = l->next)
        if(l->ptr == ptr)
            break;
    return l;
}

List*
list_first(List *l) {
    for(; l && l->prev; l = l->prev);
    return l;
}

int
list_index(List *l, void *ptr) {
    int i;

    for(l = list_first(l), i = 0; l; l = l->next, i++)
        if(l->ptr == ptr)
            return i;
    return -1;
}

List*
list_insert(List *l, void *ptr, int i) {
    List *new, *tmp;

    new = emalloc(sizeof(List));
    new->prev = new->next = NULL;
    new->ptr = ptr;
    if((tmp = list_nth(l, i))) {
        new->next = tmp;
        if(tmp->prev) {
            tmp->prev->next = new;
            new->prev = tmp->prev;
        }
        tmp->prev = new;
    } else if((tmp = list_last(l))) {
        new->prev = tmp;
        tmp->next = new;
    }
    return list_first(new);
}

List*
list_last(List *l) {
    for(; l && l->next; l = l->next);
    return l;
}

unsigned int
list_len(List *l) {
    unsigned int i;

    for(l = list_first(l), i = 0; l; l = l->next, i++);
    return i;
}

List*
list_nth(List *l, unsigned int index) {
    unsigned int i;

    for(l = list_first(l), i = 0; l; l = l->next, i++)
        if(index == i)
            break;
    return l;
}

List*
list_remove(List *l, void *ptr) {
    List *tmp;

    for(tmp = list_first(l); tmp; tmp = tmp->next) {
        if(tmp->ptr != ptr)
            continue;
        if(tmp->prev)
            tmp->prev->next = tmp->next;
        if(tmp->next)
            tmp->next->prev = tmp->prev;
        if(l == tmp)
            l = l->next ? l->next : l->prev;
        free(tmp);
        break;
    }
    return list_first(l);
}
