// readconfs.c [ 0.0.1 ]

/* *********************** Read Config File ************************ */

void read_keys_file() {
    FILE *keyfile ;
    char buffer[256];
    char dummy[256];
    char *dummy2, *dummy3, *dummy4;
    keycount = cmdcount = 0;
    int i, j;

    keyfile = fopen( KEY_FILE, "r" ) ;
    if ( keyfile == NULL ) {
        fprintf(stderr, "\033[0;34m snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,KEY_FILE);
        return;
    } else {
        while(fgets(buffer,sizeof buffer,keyfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#') continue;
            /* Now look for commands */
            if(strstr(buffer, "CMD" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy2 = strdup(dummy);
                cmds[cmdcount].name = strsep(&dummy2, ";");
                i=0;
                while(dummy2) {
                    cmds[cmdcount].list[i] = strsep(&dummy2, ";");
                    if(strcmp(cmds[cmdcount].list[i], "NULL") == 0) break;
                    i++;
                }
                cmdcount++;
                continue;
            } else if(strstr(buffer, "KEY" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1));
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                dummy3 = strsep(&dummy2, ";");
                if(strncmp(dummy3, "Alt", 3) == 0) keys[keycount].mod = Mod1Mask;
                else if(strncmp(dummy3, "CtrlAlt", 7) == 0) keys[keycount].mod = Mod1Mask|ControlMask;
                else if(strncmp(dummy3, "ShftAlt", 7) == 0) keys[keycount].mod = Mod1Mask|ShiftMask;
                else if(strncmp(dummy3, "Super", 5) == 0) keys[keycount].mod = Mod4Mask;
                else if(strncmp(dummy3, "CtrlSuper", 7) == 0) keys[keycount].mod = Mod4Mask|ControlMask;
                else if(strncmp(dummy3, "ShftSuper", 7) == 0) keys[keycount].mod = Mod4Mask|ShiftMask;
                else continue;
                keys[keycount].keysym = strsep(&dummy2, ";");
                dummy3 = strsep(&dummy2, ";");
                if(strcmp(dummy3, "kill_client") == 0) keys[keycount].myfunction = kill_client;
                else if(strcmp(dummy3, "last_desktop") == 0) keys[keycount].myfunction = last_desktop;
                else if(strcmp(dummy3, "change_desktop") == 0) {
                    keys[keycount].myfunction = change_desktop;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "follow_client_to_desktop") == 0) {
                    keys[keycount].myfunction = follow_client_to_desktop;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "client_to_desktop") == 0) {
                    keys[keycount].myfunction = client_to_desktop;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "more_master") == 0) {
                    keys[keycount].myfunction = more_master;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "move_down") == 0) {
                    keys[keycount].myfunction = move_down;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "move_up") == 0) {
                    keys[keycount].myfunction = move_up;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "switch_mode") == 0) {
                    keys[keycount].myfunction = switch_mode;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "resize_master") == 0) {
                    keys[keycount].myfunction = resize_master;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "resize_stack") == 0) {
                    keys[keycount].myfunction = resize_stack;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "rotate_desktop") == 0) {
                    keys[keycount].myfunction = rotate_desktop;
                    keys[keycount].arg.i = atoi(strsep(&dummy2, ";"));
                } else if(strcmp(dummy3, "quit") == 0) keys[keycount].myfunction = quit;
                else if(strcmp(dummy3, "next_win") == 0) keys[keycount].myfunction = next_win;
                else if(strcmp(dummy3, "prev_win") == 0) keys[keycount].myfunction = prev_win;
                else if(strcmp(dummy3, "swap_master") == 0) keys[keycount].myfunction = swap_master;
                else if(strcmp(dummy3, "update_config") == 0) keys[keycount].myfunction = update_config;
                else if(strcmp(dummy3, "spawn") == 0) {
                    keys[keycount].myfunction = spawn;
                    dummy4 = strsep(&dummy2, ";");
                    for(i=0;i<cmdcount;i++) {
                        if(strcmp(dummy4, cmds[i].name) == 0) {
                            j=0;
                            while(strcmp(cmds[i].list[j], "NULL") != 0) {
                                keys[keycount].arg.com[j] = cmds[i].list[j];
                                j++;
                            }
                            break;
                        }
                    }
                }
                keycount++;
                continue;
            }
        }
    }
    fclose(keyfile);
}

void read_rcfile() {
    FILE *rcfile ;
    char buffer[256]; /* Way bigger that neccessary */
    char dummy[256];
    char *dummy2, *dummy3;
    int i; wspccount = 0;

    rcfile = fopen( RC_FILE, "r" ) ;
    if ( rcfile == NULL ) {
        fprintf(stderr, "\033[0;34m:: snapwm : \033[0;31m Couldn't find %s\033[0m \n" ,RC_FILE);
        set_defaults();
        return;
    } else {
        while(fgets(buffer,sizeof buffer,rcfile) != NULL) {
            /* Check for comments */
            if(buffer[0] == '#') continue;
            /* Now look for info */
            if(strstr(buffer, "WINDOWTHEME" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy[strlen(dummy)-1] = '\0';
                dummy2 = strdup(dummy);
                for(i=0;i<2;i++) {
                    dummy3 = strsep(&dummy2, ";");
                    if(getcolor(dummy3) == 1) {
                        theme[i].wincolor = getcolor(defaultwincolor[i]);
                        logger("Using Default colour");
                    } else
                        theme[i].wincolor = getcolor(dummy3);
                }
            } else if(strstr(buffer, "DESKTOPS" ) != NULL) {
                DESKTOPS = atoi(strstr(buffer, " ")+1);
                if(DESKTOPS > 10) DESKTOPS = 10;
            } else if(strstr(buffer, "UPDATEINFO" ) != NULL) {
                doinfo = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "UF_WIN_ALPHA" ) != NULL) {
                ufalpha = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "BORDERWIDTH" ) != NULL) {
                bdw = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "MASTERSIZE" ) != NULL) {
                msize = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "BAR_HEIGHT" ) != NULL) {
                bar_height = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "TOPBAR" ) != NULL) {
                topbar = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "ATTACHASIDE" ) != NULL) {
                attachaside = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "TOPSTACK" ) != NULL) {
                top_stack = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "FOLLOWMOUSE" ) != NULL) {
                followmouse = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "CLICKTOFOCUS" ) != NULL) {
                clicktofocus = atoi(strstr(buffer, " ")+1);
            } else if(strstr(buffer, "DEFAULTMODE" ) != NULL) {
                mode = atoi(strstr(buffer, " ")+1);
                for(i=0;i<DESKTOPS;i++)
                    if(desktops[i].head == NULL)
                        desktops[i].mode = mode;
            } else if(strstr(buffer, "WORKSPACE" ) != NULL) {
                strncpy(dummy, strstr(buffer, " ")+1, strlen(strstr(buffer, " ")+1)-1);
                dummy2 = strdup(dummy);
                wspc[wspccount].class = strsep(&dummy2, ";");
                wspc[wspccount].desk = atoi(strsep(&dummy2, ";"));
                wspc[wspccount].follow = atoi(strsep(&dummy2, ";"));
                wspccount++;
            }
        }
        fclose(rcfile);
    }
    sh = (XDisplayHeight(dis,screen) - (bar_height+bdw));
    sw = XDisplayWidth(dis,screen)- bdw;
    return;
}

void set_defaults() {
    int i;

    logger("\033[0;32m Setting default values");
    for(i=0;i<2;i++)
        theme[i].wincolor = getcolor(defaultwincolor[i]);

    sh = (XDisplayHeight(dis,screen) - bdw);
    sw = XDisplayWidth(dis,screen)- bdw;
    return;
}

void update_config() {
    unsigned int i, old_desktops = DESKTOPS, tmp = current_desktop;
    
    XUngrabKey(dis, AnyKey, AnyModifier, root);
    read_rcfile();
    if(DESKTOPS < old_desktops) {
        save_desktop(current_desktop);
        Arg a = {.i = DESKTOPS-1};
        for(i=DESKTOPS;i<old_desktops;i++) {
            select_desktop(i);
            if(head != NULL) {
                while(desktops[current_desktop].numwins > 0) {
                    client_to_desktop(a);
                }
            }
        }
        select_desktop(tmp);
        if(current_desktop > (DESKTOPS-1)) change_desktop(a);
    }
    for(i=0;i<DESKTOPS;i++) {
        desktops[i].master_size = (sw*msize)/100;
    }
    select_desktop(current_desktop);
    Arg a = {.i = mode}; switch_mode(a);
    tile();
    update_current();

    memset(keys, 0, keycount * sizeof(key));
    memset(cmds, 0, cmdcount * sizeof(Commands));
    memset(wspc, 0, wspccount * sizeof(WORKSPACE));
    grabkeys();
}
