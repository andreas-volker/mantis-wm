struct { char type, *id; void *var; } vars[] = {
    { 'c', "border_in",     &var.border_in      },
    { 'c', "border_out",    &var.border_out     },
    { 'c', "border_urgent", &var.border_urgent  },
    { 'c', "layout",        &var.layout         },
    { 'c', "sinks",         &var.sinks          },
    { 'c', "floats",        &var.floats         },
    { 'i', "focus",         &var.focus          },
    { 'i', "newwin",        &var.newwin         },
    { 'i', "tags",          &var.tags           },
    { 'i', "gap_win",       &var.gap_win        },
    { 'i', "gap_top",       &var.gap_top        },
    { 'i', "gap_left",      &var.gap_left       },
    { 'i', "gap_right",     &var.gap_right      },
    { 'i', "gap_bottom",    &var.gap_bottom     },
    { 'i', "border_width",  &var.border_width   },
    { 'i', "follow_mouse",  &var.follow_mouse   },
    { 'm', "follow_mod",    &var.follow_mod     },
}; /* c_har, i_nt, m_od */

void
var_init(void) {
    var.sinks = estrdup("DESKTOP,DOCK,STICKY,HIDDEN");
    var.floats = estrdup("TOOLBAR,MENU,UTILITY,SPLASH,DIALOG,MODAL");
    var.layout = estrdup("right-stack");
    var.border_in = estrdup("#00F000");
    var.border_out = estrdup("#404040");
    var.border_urgent = estrdup("#F00000");
    var.tags = 4;
    var.gap_win = 2;
    var.gap_top = 0;
    var.gap_left = 0;
    var.gap_right = 0;
    var.gap_bottom = 0;
    var.border_width = 1;
    var.focus = +1;
    var.newwin = +1;
    var.follow_mouse = 1;
    var.follow_mod = Mod1Mask|Mod4Mask;
}

void
var_free(void) {
    unsigned int i;

    for(i = 0; i < LEN(vars); i++) {
        if(vars[i].type == 'c')
            free(*((char**)vars[i].var));
    }
}

char*
var_set(char *varstr, char **argv) {
    int i;
    Bool b;
    char c;
    void *ptr;
    unsigned int j, m;

    for(j = 0, ptr = NULL, c = '\0'; j < LEN(vars); j++)
        if(!strcmp(vars[j].id, varstr)) {
            c = vars[j].type;
            ptr = vars[j].var;
            break;
        }
    if(!ptr)
        return "Not a valid variable.";
    for(i = 0, b = False; argv[i]; i++) {
        if(c == 'c') {
            if(b) {
                free(*((char**)ptr));
                *((char**)ptr) = estrdup(argv[i]);
                break;
            } else if(!strcmp(*((char**)ptr), argv[i]))
                b = True;
        } else if(c == 'i') {
            if(b) {
                *((int*)ptr) = atoi(argv[i]);
                break;
            } else if(*((int*)ptr) == atoi(argv[i]))
                b = True;
        } else if(c == 'm') {
            rc_mask(argv[i], &m, NULL);
            if(b) {
                rc_mask(argv[i], (unsigned int*)ptr, NULL);
                break;
            } else if(m == *((unsigned int*)ptr))
                b = True;
        }
        if(!argv[i + 1]) {
            b = True;
            i = -1;
        }
    }
    return NULL;
}
