/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

 /*
 * Some stolen from yeahconsole -- loving that open source :)
 *
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilda.h"
#include "key_grabber.h"
#include "../tilda-config.h"

Display *dpy;
Window root;
Window win;
Window termwin;
Window last_focused;
int screen;
KeySym key;

void pull (struct tilda_window_ *tw)
{
#ifdef DEBUG
    puts("pull");
#endif

    gint w, h;
    static gint pos=0;

    if (pos == 0)
    {
        gdk_threads_enter();
        
        pos++;
        
        if (gtk_window_is_active ((GtkWindow *) tw->window) == FALSE)
            gtk_window_present ((GtkWindow *) tw->window);
        else
            gtk_widget_show ((GtkWidget *) tw->window);
            
        if (cfg_getbool (tw->tc, "pinned"))
            gtk_window_stick (GTK_WINDOW (tw->window));

        if (cfg_getbool (tw->tc, "above"))
            gtk_window_set_keep_above (GTK_WINDOW (tw->window), TRUE);

        gtk_window_move ((GtkWindow *) tw->window, cfg_getint (tw->tc, "x_pos"), cfg_getint (tw->tc, "y_pos"));
        gtk_window_resize ((GtkWindow *) tw->window, cfg_getint (tw->tc, "max_width"), cfg_getint (tw->tc, "max_height"));
        
        gdk_window_focus (tw->window->window, gtk_get_current_event_time ());
        
        gdk_flush ();
        gdk_threads_leave();
    }
    else 
    {
        gdk_threads_enter();
        
        pos--;
        
        gtk_window_resize ((GtkWindow *) tw->window, cfg_getint (tw->tc, "min_width"), cfg_getint (tw->tc, "min_height"));
        gtk_widget_hide ((GtkWidget *) tw->window);

        gdk_flush ();
        gdk_threads_leave();
    }
}

void key_grab (tilda_window *tw)
{
#ifdef DEBUG
    puts("key_grab");
#endif

    XModifierKeymap *modmap;
    gchar tmp_key[32];
    unsigned int numlockmask = 0;
    unsigned int modmask = 0;
    gint i, j;

    g_strlcpy (tmp_key, cfg_getstr (tw->tc, "key"), sizeof (tmp_key));

    /* Key grabbing stuff taken from yeahconsole who took it from evilwm */
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < modmap->max_keypermod; j++)
        {
            if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock))
            {
                numlockmask = (1 << i);
            }
        }
    }

    XFreeModifiermap(modmap);

    if (strstr(tmp_key, "Control"))
        modmask = modmask | ControlMask;

    if (strstr(tmp_key, "Alt"))
        modmask = modmask | Mod1Mask;

    if (strstr(tmp_key, "Win"))
        modmask = modmask | Mod4Mask;

    if (strstr(tmp_key, "None"))
        modmask = 0;

    if (!strstr(tmp_key, "+")) 
        perror ("Key Incorrect -- Read the README or tilda.sf.net for info, rerun as 'tilda -C' to set keybinding\n");
    
    if (strtok(tmp_key, "+"))
        key = XStringToKeysym(strtok(NULL, "+"));

    XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);

    if (numlockmask)
    {
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | modmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);
    }
}

void *wait_for_signal (tilda_window *tw)
{
    KeySym grabbed_key;
    XEvent event;

    if (!(dpy = XOpenDisplay(NULL)))
        fprintf (stderr, "Shit -- can't open Display %s", XDisplayName(NULL));

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    key_grab (tw);

    if (cfg_getbool (tw->tc, "down"))
        pull (tw);
    else
    {
        /*gdk_threads_enter();
        gtk_widget_hide (tw->window);
        gdk_flush ();
        gdk_threads_leave();*/
        pull (tw);
        pull (tw);
    }

    for (;;)
    {
        XNextEvent(dpy, &event);

        switch (event.type)
        {
            case KeyPress:
                grabbed_key = XKeycodeToKeysym(dpy, event.xkey.keycode, 0);

                if (key == grabbed_key)
                {
                    pull (tw);
                    break;
                }
            default:
                break;
        }
    }

    return 0;
}

