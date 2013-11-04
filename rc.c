void
rc_init(void) {
    int line, ignore, i;
    char buf[RCLEN + 2], *c;
    FILE *f;

    c = parsepath(RCPATH);
    if(!(f = fopen(c, "r")) && !(f = fopen(RCSAMPLE, "r")))
        eprintf(PROGRAM": "RCSAMPLE": No such file or directory\n");
    free(c);
    for(line = 1, ignore = 0; fgets(buf, RCLEN + 2, f); line++) {
        if(buf[strlen(buf) - 1] != '\n') {
            if(ignore != line)
                fprintf(stderr, PROGRAM": "RCPATH": line %d: "
                                "Exceed the %d chars limit.\n", line, RCLEN);
            ignore = line--;
            continue;
        }
        for(i = 0; buf[i] != '#' && buf[i] != '\n';)
            if(buf[++i] == '\'')
                while(buf[++i] != '\'');
        for(--i; i >= 0 && (buf[i] == ' ' || buf[i] == '\t'); i--);
        for(; ++i < (RCLEN + 1); (buf[i] = '\0'));
        if(ignore == line || !buf[0])
            continue;
        c = NULL;
        if((strstr(buf, "layout") && buf[strlen(buf) - 1] == ':') ||
           isspace(buf[0]))
            c = parselayout(buf);
        else
            c = parseline(buf);
        if(c)
            fprintf(stderr, PROGRAM": "RCPATH": line %d: %s\n", line, c);
    }
    fclose(f);
    data.tagmask = 0L;
    if(!var.tags)
        var.tags = 1;
    for(i = 0; i < (int)var.tags; i++)
        data.tagmask |= (1<<i);
}

void
rc_mask(char *mask, unsigned int *mod, char **key) {
    char *str;
    unsigned int i, j;

    str = estrdup(mask);
    for(i = 0, j = 2; str[i]; i++)
        if(str[i] == '-')
            ++j;
    char *t[j];
    *mod = getmodmask((t[0] = strtok(str, "-")));
    for(i = 1; (t[i] = strtok(NULL, "-")); i++)
        *mod |= getmodmask(t[i]);
    if(key)
        *key = estrdup(t[i - 1]);
    free(str);
}

unsigned int
getmodmask(char *c) {
    switch(c[0]) {
    case 'C': return ControlMask;
    case 'S': return ShiftMask;
    case 'M': return getmodnumber(c[1]);
    default : return 0;
    }
}

unsigned int
getmodnumber(char c) {
    switch(c) {
    case '1': return Mod1Mask;
    case '4': return Mod4Mask;
    case '5': return Mod5Mask;
    default : return 0;
    }
}

char*
parselayout(char *str) {
    int i, j;
    char *c;

    if(isspace(str[0])) {
        for(i = 0, j = 1; str[i]; i++)
            if(str[i] == ';')
                ++j;
        for(i = 0; isspace(str[i]); i++);
        return layout_add(NULL, str + i, j);
    } else if((c = strchr(str, '\''))) {
        for(i = 1; c[i] != '\''; i++);
        c[i] = '\0';
        return layout_add(++c, NULL, 1);
    } else
        return "Layout name must be \'single quoted\'";
}

char*
parseline(char *str) {
    int i, j, k;
    Bool b;

    if(!str)
        return "Nothing to parse.";
    for(i = 0, j = 2, b = False; str[i]; i++) {
        if(str[i] == '\'')
            if(!(b = !b))
                ++j;
        if(!b && !isalnum(str[i]) && isalnum(str[i + 1]))
            ++j;
    }
    char t[j][RCLEN + 2], *v[j], start[j], end[j];
    for(i = 0; i < j; i++) {
        memset(&(t[i]), '\0', sizeof(t[i]));
        start[i] = end[i] = '\0';
        v[i] = NULL;
    }
    for(i = 0, j = 0; str[i]; i++) {
        if(!isalnum(str[i]) && str[i] != '\'' && str[i] != '<')
            continue;
        if(!isalnum((start[j] = str[i])))
            ++i;
        end[j] = start[j] == '\'' ? '\'' : (start[j] == '<'  ? '>'  : ',');
        if(isdigit(str[i]) && str[i - 1] == '-')
            --i;
        for(k = 0; str[i] && str[i] != end[j]; i++, k++) {
            if(isalnum(start[j]) && isspace(str[i]))
                break;
            t[j][k] = str[i];
        }
        t[j][k] = '\0';
        v[j] = t[j];
        v[++j] = NULL;
    }
    return setparsed(start, v);
}

char*
parsepath(char *str) {
    int i, j;
    char *c, tmp[BUFSIZ];

    c = estrdup(str);
    for(i = 0; c[i]; i++) {
        if(c[i] != '$')
            continue;
        c[i] = '\0';
        memset(&tmp, '\0', sizeof(tmp));
        for(j = 0, i++; isalnum(c[i]) || c[i] == '_'; j++, i++)
            tmp[j] = c[i];
        sprintf(tmp, "%s%s%s", c, getenv(tmp) ? getenv(tmp) : "", c + i);
        free(c);
        c = estrdup(tmp);
        i = -1;
    }
    return c;
}

char*
setparsed(char *c, char **v) {
    char type[2], *argv[2];
    unsigned int i;

    if(!v[0] || !v[1])
        return "Nothing to parse.";
    if(isalpha(c[0])) {
        argv[0] = v[1];
        argv[1] = NULL;
        return var_set(v[0], argv);
    } else if(c[0] != '<')
        return "Invalid first char.";
    if(strstr(v[0], "Button"))
        return button_add(v[0], v[1]);
    memset(&type, '-', sizeof(type));
    for(i = 0; v[i] && i < LEN(type); i++)
        type[i] = (c[i + 2] == '\'' ? 'c' : (isdigit(c[i + 2]) ? 'i' :
                  (c[i + 2] == '<'  ? 'm' : (isalpha(c[i + 2]) ? 'n' : '-'))));
    for(i = 0; i < LEN(argv); i++)
        argv[i] = v[i + 2];
    return key_add(v[0], v[1], &argv, &type);
}
