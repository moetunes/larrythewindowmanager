/* larry.c.0.0.1 */

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))
#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

typedef union {
    const char** com;
    const int i;
} Arg;

// Structs
typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
} key;

typedef struct client client;
struct client{
    // Prev and next client
    client *next;
    client *prev;
    // The window
    Window win;
    unsigned int x, y, width, height;
};

typedef struct desktop desktop;
struct desktop{
    int master_size;
    int mode, growth, numwins;
    client *head;
    client *current;
    client *transient;
};

typedef struct {
    const char *class;
    int preferredd;
    int followwin;
} Convenience;

static void do_Events();
static void grabkeys();
static void quit();
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
//event functions
static void keypress(XEvent *e);
static void maprequest(XEvent *e);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void configurerequest(XEvent *e);
static void motionnotify(XEvent *e);
static void enternotify(XEvent *e);
static void unmapnotify(XEvent *e);
static void destroynotify(XEvent *e);

static void add_window(Window w, int tw);
static void remove_window(Window w, int dr);
static void next_win();
static void prev_win();
static void move_down(const Arg arg);
static void move_up(const Arg arg);
static void move_right(const Arg arg);
static void move_left(const Arg arg);
static void swap_master();
static void change_desktop(const Arg arg);
static void last_desktop();
static void logger(const char* e);
static void rotate_desktop(const Arg arg);
static void follow_client_to_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void save_desktop(int i);
static void select_desktop(int i);
static void tile();
static void update_current();
static void switch_mode(const Arg arg);
static void resize_master(const Arg arg);
static void resize_stack(const Arg arg);
static void toggle_panel();
static void kill_client();
static void kill_client_now(Window w);
static void warp_pointer();

static Display * dis;
static Window root;
static client *head;
static client *current;
static client *transient;
static XWindowAttributes attr;
static XButtonEvent start;
//static XEvent ev;

#include "config.h"

static int current_desktop;
static int previous_desktop;
static int growth;
static int master_size;
static int mode;
static int panel_size;
static unsigned int win_focus;
static unsigned int win_unfocus;

unsigned int numlockmask;        /* dynamic key lock mask */
static int screen;
static int sh;
static int stop_larry;
static int sw;
static int xerror(Display *dis, XErrorEvent *ee);
static int (*xerrorxlib)(Display *, XErrorEvent *);

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [EnterNotify] = enternotify,
    [UnmapNotify] = unmapnotify,
    [ButtonPress] = buttonpress,
    [ButtonRelease] = buttonrelease,
    [DestroyNotify] = destroynotify,
//    [ConfigureNotify] = configurenotify,
    [ConfigureRequest] = configurerequest,
    [MotionNotify] = motionnotify
};

// Desktop array
static desktop desktops[DESKTOPS];

/* ***************************** Window Management ******************************* */
void add_window(Window w, int tw) {
    client *c,*t;

    if(tw < 2) // For shifting windows between desktops
        if(!(c = (client *)calloc(1,sizeof(client)))) {
            logger("\033[0;31mError calloc!");
            exit(1);
        }

    if(tw == 1) { // For the transient window
        if(transient == NULL) {
            c->prev = NULL; c->next = NULL;
            c->win = w; transient = c;
        } else {
            t=transient;
            c->prev = NULL; c->next = t;
            c->win = w; t->prev = c;
            transient = c;
        }
        save_desktop(current_desktop);
        return;
    }

    if(tw == 0) {
        XGetWindowAttributes(dis, w, &attr);
        XMoveWindow(dis, w,sw/2-(attr.width/2),sh/2-(attr.height/2));
        XGetWindowAttributes(dis, w, &attr);
        c->x = attr.x;
        if(TOP_PANEL == 0 && attr.y < panel_size) c->y = panel_size;
        else c->y = attr.y;
        c->width = attr.width;
        c->height = attr.height;
    }
    if(head == NULL) {
        c->next = NULL; c->prev = NULL;
        c->win = w; head = c;
    } else {
        if(ATTACH_ASIDE == 0) {
            if(TOP_STACK == 0) {
                if(head->next == NULL) {
                    c->prev = head; c->next = NULL;
                } else {
                    t = head->next;
                    c->prev = t->prev; c->next = t;
                    t->prev = c;
                }
                c->win = w; head->next = c;
            } else {
                for(t=head;t->next;t=t->next); // Start at the last in the stack
                c->next = NULL; c->prev = t;
                c->win = w; t->next = c;
            }
        } else {
            t=head;
            c->prev = NULL; c->next = t;
            c->win = w; t->prev = c;
            head = c; current = head;
            warp_pointer();
        }
    }

    current = head;
    desktops[current_desktop].numwins += 1;
    if(growth > 0) growth = growth*(desktops[current_desktop].numwins-1)/desktops[current_desktop].numwins;
    else growth = 0;
    save_desktop(current_desktop);
    // for folow mouse
    if(FOLLOW_MOUSE == 0) XSelectInput(dis, c->win, EnterWindowMask);
}

void remove_window(Window w, int dr) {
    client *c;

    if(transient != NULL) {
        for(c=transient;c;c=c->next) {
            if(c->win == w) {
                if(c->prev == NULL && c->next == NULL) transient = NULL;
                else if(c->prev == NULL) {
                    transient = c->next; c->next->prev = NULL;
                }
                else if(c->next == NULL) {
                    c->prev->next = NULL;
                }
                else {
                    c->prev->next = c->next; c->next->prev = c->prev;
                }
                free(c);
                save_desktop(current_desktop);
                return;
            }
        }
    }

    for(c=head;c;c=c->next) {
        if(c->win == w) {
            if(desktops[current_desktop].numwins < 4) growth = 0;
            else growth = growth*(desktops[current_desktop].numwins-1)/desktops[current_desktop].numwins;
            XUnmapWindow(dis, c->win);
            if(c->prev == NULL && c->next == NULL) {
                head = NULL; current = NULL;
            } else if(c->prev == NULL) {
                head = c->next; c->next->prev = NULL;
                current = c->next;
            } else if(c->next == NULL) {
                c->prev->next = NULL; current = c->prev;
            } else {
                c->prev->next = c->next; c->next->prev = c->prev;
                current = c->prev;
            }

            if(dr == 0) free(c);
            desktops[current_desktop].numwins -= 1;
            save_desktop(current_desktop);
            tile();
            update_current();
            if(desktops[current_desktop].numwins > 1 || mode == 3) warp_pointer();
            return;
        }
    }
}

void next_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->next == NULL) c = head;
        else c = current->next;

        current = c;
        if(mode == 1)
            tile();
        update_current(); warp_pointer();
    }
}

void prev_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->prev == NULL) for(c=head;c->next;c=c->next);
        else c = current->prev;

        current = c;
        if(mode == 1)
            tile();
        update_current();
        warp_pointer();
    }
}

void move_down(const Arg arg) {
    if(mode == 3 && current != NULL) {
        current->y += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
        return;
    }
    Window tmp;
    if(current == NULL || current->next == NULL || current->win == head->win || current->prev == NULL)
        return;

    tmp = current->win;
    current->win = current->next->win;
    current->next->win = tmp;
    //keep the moved window activated
    next_win();
    save_desktop(current_desktop);
    tile();
}

void move_up(const Arg arg) {
    if(mode == 3 && current != NULL) {
        current->y += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
        return;
    }
    Window tmp;
    if(current == NULL || current->prev == head || current->win == head->win) {
        return;
    }
    tmp = current->win;
    current->win = current->prev->win;
    current->prev->win = tmp;
    prev_win();
    save_desktop(current_desktop);
    tile();
}

void move_left(const Arg arg) {
    if(mode == 3 && current != NULL) {
        current->x += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
    }
}

void move_right(const Arg arg) {
    if(mode == 3 && current != NULL) {
        current->x += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
    }
}

void swap_master() {
    Window tmp;

    if(head->next != NULL && current != NULL && mode != 1) {
        if(current == head) {
            tmp = head->next->win; head->next->win = head->win;
            head->win = tmp;
        } else {
            tmp = head->win; head->win = current->win;
            current->win = tmp; current = head;
        }
        save_desktop(current_desktop);
        tile();
        update_current();
    }
}

/* **************************** Desktop Management ************************************* */
void change_desktop(const Arg arg) {
    client *c;

    if(arg.i == current_desktop)
        return;

    // Save current "properties"
    save_desktop(current_desktop); previous_desktop = current_desktop;

    // Unmap all window
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            XUnmapWindow(dis,c->win);
    if(head != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis,c->win);

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    // Map all windows
    if(transient != NULL)
        for(c=transient;c;c=c->next)
            XMapWindow(dis,c->win);
    if(head != NULL) {
        if(mode != 1) {
            XMapWindow(dis,current->win);
            for(c=head;c;c=c->next)
                if(c != current) XMapWindow(dis,c->win);
        }
    }

    tile();
    update_current();
    warp_pointer();
}

void last_desktop() {
    Arg a = {.i = previous_desktop};
    change_desktop(a);
}

void rotate_desktop(const Arg arg) {
    Arg a = {.i = (current_desktop + TABLENGTH(desktops) + arg.i) % TABLENGTH(desktops)};
     change_desktop(a);
}

void follow_client_to_desktop(const Arg arg) {
    client_to_desktop(arg);
    change_desktop(arg);
}

void client_to_desktop(const Arg arg) {
    client *tmp = current;
    int tmp2 = current_desktop;

    if(arg.i == current_desktop || current == NULL)
        return;

    // Remove client from current desktop
    XUnmapWindow(dis,current->win);
    remove_window(current->win, 1);

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->win, 2);
    save_desktop(arg.i);

    select_desktop(tmp2);
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].mode = mode;
    desktops[i].growth = growth;
    desktops[i].head = head;
    desktops[i].current = current;
    desktops[i].transient = transient;
}

void select_desktop(int i) {
    master_size = desktops[i].master_size;
    mode = desktops[i].mode;
    growth = desktops[i].growth;
    head = desktops[i].head;
    current = desktops[i].current;
    transient = desktops[i].transient;
    current_desktop = i;
}

void tile() {
    client *c;
    int n = 0,x = 0, y = 0;

    // For a top panel
    if(TOP_PANEL == 0) y = panel_size;

    // If only one window
    if(mode != 3 && head != NULL && head->next == NULL) {
        if(mode == 1) XMapWindow(dis, current->win);
        XMoveResizeWindow(dis,head->win,0,y,sw+2*BORDER_WIDTH,sh+2*BORDER_WIDTH);
    }

    else if(head != NULL) {
        switch(mode) {
            case 0: /* Vertical */
            	// Master window
                XMoveResizeWindow(dis,head->win,0,y,master_size - BORDER_WIDTH,sh - BORDER_WIDTH);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                XMoveResizeWindow(dis,head->next->win,master_size + BORDER_WIDTH,y,sw-master_size-(2*BORDER_WIDTH),(sh/n)+growth - BORDER_WIDTH);
                y += (sh/n)+growth;
                for(c=head->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,master_size + BORDER_WIDTH,y,sw-master_size-(2*BORDER_WIDTH),(sh/n)-(growth/(n-1)) - BORDER_WIDTH);
                    y += (sh/n)-(growth/(n-1));
                }
                break;
            case 1: /* Fullscreen */
                XMoveResizeWindow(dis,current->win,0,y,sw+2*BORDER_WIDTH,sh+2*BORDER_WIDTH);
                XMapWindow(dis, current->win);
                break;
            case 2: /* Horizontal */
            	// Master window
                XMoveResizeWindow(dis,head->win,0,y,sw-BORDER_WIDTH,master_size - BORDER_WIDTH);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                XMoveResizeWindow(dis,head->next->win,0,y+master_size + BORDER_WIDTH,(sw/n)+growth-BORDER_WIDTH,sh-master_size-(2*BORDER_WIDTH));
                x = (sw/n)+growth;
                for(c=head->next->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,x,y+master_size + BORDER_WIDTH,(sw/n)-(growth/(n-1)) - BORDER_WIDTH,sh-master_size-(2*BORDER_WIDTH));
                    x += (sw/n)-(growth/(n-1));
                }
                break;
            case 3: // Stacking
                for(c=head;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,c->x,c->y,c->width,c->height);
                }
                break;
            default:
                break;
        }
    }
}

void update_current() {
    client *c;

    for(c=head;c;c=c->next) {
        if((head->next == NULL) || (mode == 1))
            XSetWindowBorderWidth(dis,c->win,0);
        else
            XSetWindowBorderWidth(dis,c->win,BORDER_WIDTH);

        if(current == c && transient == NULL) {
            // "Enable" current window
            XSetWindowBorder(dis,c->win,win_focus);
            XSetInputFocus(dis,c->win,RevertToParent,CurrentTime);
            XRaiseWindow(dis,c->win);
            if(CLICK_TO_FOCUS == 0)
                XUngrabButton(dis, AnyButton, AnyModifier, c->win);
        }
        else {
            XSetWindowBorder(dis,c->win,win_unfocus);
            if(CLICK_TO_FOCUS == 0)
                XGrabButton(dis, AnyButton, AnyModifier, c->win, True, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
    }
    if(transient != NULL) {
        XSetInputFocus(dis,transient->win,RevertToParent,CurrentTime);
        XRaiseWindow(dis,transient->win);
    }
    XSync(dis, False);
}

void switch_mode(const Arg arg) {
    client *c;

    if(mode == arg.i) return;
    growth = 0;
    if(mode == 1 && current != NULL && head->next != NULL) {
        XUnmapWindow(dis, current->win);
        for(c=head;c;c=c->next)
            XMapWindow(dis, c->win);
    }

    mode = arg.i;
    if(mode == 0 || mode == 3) master_size = sw * MASTER_SIZE;
    if(mode == 1 && current != NULL && head->next != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis, c->win);

    if(mode == 2) master_size = sh * MASTER_SIZE;
    tile();
    update_current();
    warp_pointer();
}

void resize_master(const Arg arg) {
    if(mode == 3 && current != NULL) {
        current->width += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height);
    } else if(arg.i > 0) {
        if((mode != 2 && sw-master_size > 70) || (mode == 2 && sh-master_size > 70))
            master_size += arg.i;
    } else if(master_size > 70) master_size += arg.i;
    tile();
}

void resize_stack(const Arg arg) {
    if(mode == 3 && current != NULL) {
        current->height += arg.i;
        XMoveResizeWindow(dis,current->win,current->x,current->y,current->width,current->height+arg.i);
    } else if(desktops[current_desktop].numwins > 2) {
        int n = desktops[current_desktop].numwins-1;
        if(arg.i >0) {
            if((mode != 2 && sh-(growth+sh/n) > (n-1)*70) || (mode == 2 && sw-(growth+sw/n) > (n-1)*70))
                growth += arg.i;
        } else {
            if((mode != 2 && (sh/n+growth) > 70) || (mode == 2 && (sw/n+growth) > 70))
                growth += arg.i;
        }
        tile();
    }
}

void toggle_panel() {
    if(PANEL_HEIGHT > 0) {
        if(panel_size >0) {
            sh += panel_size;
            panel_size = 0;
        } else {
            panel_size = PANEL_HEIGHT;
            sh -= panel_size;
        }
        tile();
    }
}
/* ******************************************************************* */
void grabkeys() {
    int i, j;
    XModifierKeymap *modmap;
    KeyCode code;

    XUngrabKey(dis, AnyKey, AnyModifier, root);
    // numlock workaround
    numlockmask = 0;
    modmap = XGetModifierMapping(dis);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < modmap->max_keypermod; j++) {
            if(modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dis, XK_Num_Lock))
                numlockmask = (1 << i);
        }
    }
    XFreeModifiermap(modmap);

    // For each shortcuts
    for(i=0;i<TABLENGTH(keys);++i) {
        code = XKeysymToKeycode(dis,keys[i].keysym);
        XGrabKey(dis, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | LockMask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | numlockmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dis, code, keys[i].mod | numlockmask | LockMask, root, True, GrabModeAsync, GrabModeAsync);
    }
    for(i=1;i<4;i+=2) {
        XGrabButton(dis, i, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, Mod1Mask | LockMask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, Mod1Mask | numlockmask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dis, i, Mod1Mask | numlockmask | LockMask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    }
}

void keypress(XEvent *e) {
    static unsigned int len = sizeof keys / sizeof keys[0];
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    keysym = XkbKeycodeToKeysym(dis, (KeyCode)ev->keycode, 0, 0);
    for(i = 0; i < len; i++) {
        if(keysym == keys[i].keysym && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)) {
            if(keys[i].function)
                keys[i].function(keys[i].arg);
        }
    }
}

void warp_pointer() {
    // Move cursor to the center of the current window
    if(FOLLOW_MOUSE == 0 && head != NULL) {
        XGetWindowAttributes(dis, current->win, &attr);
        XWarpPointer(dis, None, current->win, 0, 0, 0, 0, attr.width/2, attr.height/2);
    }
}

/* ********************** Signal Management ************************** */
void configurerequest(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    int y = 0;

    wc.x = ev->x;
    wc.y = ev->y + y;
    if(ev->width < sw) wc.width = ev->width;
    else wc.width = sw;
    if(ev->height < sh-y) wc.height = ev->height;
    else wc.height = sh;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
    XSync(dis, False);
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;

    XGetWindowAttributes(dis, ev->window, &attr);
    if(attr.override_redirect) return;

    int y=0;
    if(TOP_PANEL == 0) y = panel_size;
    // For fullscreen mplayer (and maybe some other program)
    client *c;

    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            XMoveResizeWindow(dis,c->win,0,y,sw,sh-y);
            return;
        }

   	Window trans = None;
    if (XGetTransientForHint(dis, ev->window, &trans) && trans != None) {
        add_window(ev->window, 1); XMapWindow(dis, ev->window);
        if((attr.y + attr.height) > sh)
            XMoveResizeWindow(dis,ev->window,attr.x,y,attr.width,attr.height-10);
        XSetWindowBorderWidth(dis,ev->window,BORDER_WIDTH);
        XSetWindowBorder(dis,ev->window,win_focus);
        update_current();
        return;
    }

    XClassHint ch = {0};
    static unsigned int len = sizeof convenience / sizeof convenience[0];
    int i=0, j=0, tmp = current_desktop;
    if(XGetClassHint(dis, ev->window, &ch))
        for(i=0;i<len;i++)
            if(strcmp(ch.res_class, convenience[i].class) == 0) {
                save_desktop(tmp);
                select_desktop(convenience[i].preferredd-1);
                for(c=head;c;c=c->next)
                    if(ev->window == c->win)
                        ++j;
                if(j < 1) add_window(ev->window, 0);
                if(tmp == convenience[i].preferredd-1) {
                    XMapWindow(dis, ev->window);
                    tile();
                    update_current();
                } else select_desktop(tmp);
                if(convenience[i].followwin != 0) {
                    Arg a = {.i = convenience[i].preferredd-1};
                    change_desktop(a);
                }
                if(ch.res_class) XFree(ch.res_class);
                if(ch.res_name) XFree(ch.res_name);
                return;
            }

    add_window(ev->window, 0);
    tile();
    if(mode != 1) XMapWindow(dis,ev->window);
    update_current();
}

void destroynotify(XEvent *e) {
    int i = 0, tmp = current_desktop;
    client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if(transient != NULL) {
        for(c=transient;c;c=c->next)
            if(ev->window == c->win) {
                remove_window(ev->window, 0);
                return;
            }
    }
    save_desktop(tmp);
    for(i=0;i<TABLENGTH(desktops);++i) {
        select_desktop(i);
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                remove_window(ev->window, 0);
                select_desktop(tmp);
                return;
            }
    }
    select_desktop(tmp);
}

void enternotify(XEvent *e) {
    client *c;
    XCrossingEvent *ev = &e->xcrossing;

    if(FOLLOW_MOUSE == 0) {
        if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
            return;
        if(transient != NULL) return;
        for(c=head;c;c=c->next)
           if(ev->window == c->win) {
                current = c;
                update_current();
                return;
       }
   }
}

void buttonpress(XEvent *e) {
    XButtonEvent *ev = &e->xbutton;

    if(ev->subwindow == None) return;
    XGrabPointer(dis, ev->subwindow, True,
            PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
            GrabModeAsync, None, None, CurrentTime);
    XGetWindowAttributes(dis, ev->subwindow, &attr);
    start = e->xbutton;
}

void motionnotify(XEvent *e) {
    int xdiff, ydiff;
    XMotionEvent *ev = &e->xmotion;

    while(XCheckTypedEvent(dis, MotionNotify, e));
    xdiff = ev->x_root - start.x_root;
    ydiff = ev->y_root - start.y_root;
    XMoveResizeWindow(dis, ev->window,
        attr.x + (start.button==1 ? xdiff : 0),
        attr.y + (start.button==1 ? ydiff : 0),
        MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
        MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
}

void buttonrelease(XEvent *e) {
    client *c;
    XButtonEvent *ev = &e->xbutton;

    XUngrabPointer(dis, CurrentTime);
    if(mode == 3)
        for(c=head;c;c=c->next)
            if(ev->window == c->win) {
                XGetWindowAttributes(dis, c->win, &attr);
                c->x = attr.x;
                c->y = attr.y;
                c->width = attr.width;
                c->height = attr.height;
            }
}

void unmapnotify(XEvent *e) { // for thunderbird's write window and maybe others
    XUnmapEvent *ev = &e->xunmap;
    int i = 0, tmp = current_desktop;
    client *c;

    if(ev->send_event == 1) {
        save_desktop(tmp);
        for(i=0;i<TABLENGTH(desktops);++i) {
            select_desktop(i);
            for(c=head;c;c=c->next)
                if(ev->window == c->win) {
                    remove_window(ev->window, 1);
                    select_desktop(tmp);
                    return;
                }
        }
        select_desktop(tmp);
    }
}

/* ***************************************************************** */

void logger(const char* e) {
    fprintf(stderr,"\n\033[0;34m:: dminiwm : %s \033[0;m\n", e);
}

void spawn(const Arg arg) {
    if(fork() == 0) {
        if(fork() == 0) {
            if(dis) close(ConnectionNumber(dis));
            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }
        exit(0);
    }
}

void kill_client() {
    if(head == NULL) return;
    kill_client_now(current->win);
    remove_window(current->win, 0);
}

void kill_client_now(Window w) {
    Atom *protocols, wm_delete_window;
    int n, i, can_delete = 0;
    XEvent ke;
    wm_delete_window = XInternAtom(dis, "WM_DELETE_WINDOW", True); 

    if (XGetWMProtocols(dis, w, &protocols, &n) != 0)
        for (i=0;i<n;i++)
            if (protocols[i] == wm_delete_window) can_delete = 1;

    if(can_delete == 1) {
        ke.type = ClientMessage;
        ke.xclient.window = w;
        ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
        ke.xclient.format = 32;
        ke.xclient.data.l[0] = wm_delete_window;
        ke.xclient.data.l[1] = CurrentTime;
        XSendEvent(dis, w, False, NoEventMask, &ke);
    } else XKillClient(dis, w);
}

void quit() {
    int i;
    Window w;
    Window *childs;
    unsigned int nchilds;

    XQueryTree(dis, root, &w, &w, &childs, &nchilds);
    for (i = 0; i < nchilds; i++) {
        if (childs[i] == root) continue;
        kill_client_now(w);
    }

    stop_larry = 1;
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XAllocNamedColor(dis,map,color,&c,&c)) {
        logger("\033[0;31mError parsing color!");
        exit(1);
    }
    return c.pixel;
}

void do_Events() {
    XEvent ev;

    while(stop_larry < 1 && !XNextEvent(dis,&ev)) {
        if(events[ev.type])
            events[ev.type](&ev);
    }
}

void sigchld(int unused) {
	if(signal(SIGCHLD, sigchld) == SIG_ERR) {
		printf("\033[0;31mCan't install SIGCHLD handler\n\033[0;m");
		exit(1);
        }
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void setup() {
    int i;

    sigchld(0);

    screen = DefaultScreen(dis);
    root = DefaultRootWindow(dis);
    panel_size = PANEL_HEIGHT;
    sw = XDisplayWidth(dis,screen)-BORDER_WIDTH;
    sh = XDisplayHeight(dis,screen)-panel_size-BORDER_WIDTH;
    grabkeys();
    if(SHOW_BAR > 0) toggle_panel();
    // Colors
    win_focus = getcolor(FOCUS);
    win_unfocus = getcolor(UNFOCUS);
    // Default stack
    mode = DEFAULT_MODE;

    // Master size
    if(mode == 2) master_size = sh*MASTER_SIZE;
    else master_size = sw*MASTER_SIZE;

    // Set up all desktop
    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].master_size = master_size;
        desktops[i].mode = mode;
        desktops[i].growth = 0;
        desktops[i].numwins = 0;
        desktops[i].head = NULL;
        desktops[i].current = NULL;
        desktops[i].transient = NULL;
    }

    // Select first dekstop by default
    const Arg arg = {.i = 0};
    current_desktop = arg.i;
    change_desktop(arg);
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask);
    XSetErrorHandler(xerror);
    stop_larry = 0;
}

/* There's no way to check accesses to destroyed windows, thus those cases are ignored (especially on UnmapNotify's).  Other types of errors call Xlibs default error handler, which may call exit.  */
int xerror(Display *dis, XErrorEvent *ee) {
    if(ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    printf("\t\033[0;31mLarry Had A Bad Window Error!");
    return xerrorxlib(dis, ee); /* may call exit */
}

int main() {
    // Open display
    if(!(dis = XOpenDisplay(NULL))) {
        printf("\033[0;31mCannot open display!");
        exit(1);
    }

    setup();
    printf("\tLarry's up and Running!\n");
    do_Events();
    // Close display
    printf("\t\033[0;35mLarry says BYE\n\n\033[0;m");
    XCloseDisplay(dis);

    return 0;
}