void*
emalloc(size_t size) {
    void *ptr;

    if(!(ptr = malloc(size)))
        eprintf("Can not allocate %zu bytes.\n", size);
    return ptr;
}

void
eprintf(char *str, ...) {
    va_list l;

    va_start(l, str);
    vfprintf(stderr, str, l);
    va_end(l);
    exit(EXIT_FAILURE);
}

char*
estrdup(char *str) {
    char *c;
    size_t len;

    len = strlen(str) + 1;
    c = emalloc(len);
    return memcpy(c, str, len);
}
