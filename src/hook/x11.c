/**
 * \file src/lib/x11.c
 * \brief libX11 wrapper
 * \author Pyry Haulos <pyry.haulos@gmail.com>
 * \date 2007
 */

/* x11.c -- libX11 wrapper
 * Copyright (C) 2007 Pyry Haulos
 * For conditions of distribution and use, see copyright notice in glc.h
 */

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>

#include "../common/glc.h"
#include "../common/util.h"
#include "lib.h"
#include "../capture/gl_capture.h"

/**
 * \addtogroup lib
 *  \{
 */

/**
 * \defgroup x11 libX11 wrapper
 *  \{
 */

struct x11_private_s {
	glc_t *glc;

	void *libX11_handle;
	int (*XNextEvent)(Display *, XEvent *);
	int (*XPeekEvent)(Display *, XEvent *);
	int (*XWindowEvent)(Display *, Window, long, XEvent *);
	int (*XMaskEvent)(Display *, long, XEvent *);
	Bool (*XCheckWindowEvent)(Display *, Window, long, XEvent *);
	Bool (*XCheckMaskEvent)(Display *, long, XEvent *);
	Bool (*XCheckTypedEvent)(Display *, long, XEvent *);
	Bool (*XCheckTypedWindowEvent)(Display *, Window, int, XEvent *);

	int (*XIfEvent)(Display *, XEvent *, Bool (*)(), XPointer);
	Bool (*XCheckIfEvent)(Display *, XEvent *, Bool (*)(), XPointer);
	int (*XPeekIfEvent)(Display *, XEvent *, Bool (*)(), XPointer);

	void *libXxf86vm_handle;
	Bool (*XF86VidModeSetGamma)(Display *, int, XF86VidModeGamma *);

	unsigned int key_mask;
	KeySym capture;
	Time last_event_time;

	glc_utime_t stop;
};

#define X11_KEY_CTRL               1
#define X11_KEY_ALT                2
#define X11_KEY_SHIFT              4

__PRIVATE struct x11_private_s x11;
__PRIVATE void get_real_x11();
__PRIVATE void x11_event(Display *dpy, XEvent *event);
__PRIVATE int x11_parse_hotkey(const char *hotkey);

int x11_init(glc_t *glc)
{
	x11.glc = glc;

	get_real_x11();

	if (getenv("GLC_HOTKEY")) {
		if (x11_parse_hotkey(getenv("GLC_HOTKEY"))) {
			util_log(x11.glc, GLC_WARNING, "x11",
				 "invalid hotkey '%s'", getenv("GLC_HOTKEY"));
			util_log(x11.glc, GLC_WARNING, "x11",
				 "using default <Shift>F8\n");
			x11.key_mask = X11_KEY_SHIFT;
			x11.capture = XK_F8;
		}
	} else {
		x11.key_mask = X11_KEY_SHIFT;
		x11.capture = XK_F8;
	}

	x11.stop = util_time(x11.glc);

	return 0;
}

int x11_parse_hotkey(const char *hotkey)
{
	/* FIXME this is ugly... */
	int c, s;
	x11.key_mask = 0;
	c = s = 0;
	while (hotkey[c] != '\0') {
		if (hotkey[c] == '<')
			s = c;
		else if (hotkey[c] == '>') {
			if (!strncmp(&hotkey[s], "<Ctrl>", c - s))
				x11.key_mask |= X11_KEY_CTRL;
			else if (!strncmp(&hotkey[s], "<Shift>", c - s))
				x11.key_mask |= X11_KEY_SHIFT;
			else
				return EINVAL;
			s = c + 1;
		}
		c++;
	}

	x11.capture = XStringToKeysym(&hotkey[s]);

	if (!x11.capture)
		return EINVAL;
	return 0;
}

int x11_close()
{
	return 0;
}

void x11_event(Display *dpy, XEvent *event)
{
	int ret;
	if (!event)
		return;

	if (event->type == KeyPress) {
		if (event->xkey.keycode == XKeysymToKeycode(dpy, x11.capture)) {
			if (event->xkey.time == x11.last_event_time)
				return; /* handle duplicates */

			if ((x11.key_mask & X11_KEY_CTRL) && (!(event->xkey.state & ControlMask)))
				return;

			if ((x11.key_mask & X11_KEY_SHIFT) && (!(event->xkey.state & ShiftMask)))
				return;

			if (x11.glc->flags & GLC_CAPTURE) { /* stop */
				x11.glc->flags &= ~GLC_CAPTURE;
				x11.stop = util_time(x11.glc);
				util_log(x11.glc, GLC_INFORMATION, "x11", "stopped capturing");
			} else { /* start */
				if (!lib.running) {
					if ((ret = start_glc())) {
						util_log(x11.glc, GLC_ERROR, "x11",
							 "can't start capturing: %s (%d)",
							 strerror(ret), ret);
						return; /* don't set GLC_CAPTURE flag */
					}
				}

				util_timediff(x11.glc, util_time(x11.glc) - x11.stop);
				x11.glc->flags |= GLC_CAPTURE;
				util_log(x11.glc, GLC_INFORMATION, "x11", "started capturing");

			}
			x11.last_event_time = event->xkey.time;
		}
	}
}

void get_real_x11()
{
	if (!lib.dlopen)
		get_real_dlsym();

	x11.libX11_handle = lib.dlopen("libX11.so", RTLD_LAZY);
	if (!x11.libX11_handle)
		goto err;

	x11.XNextEvent =
	  (int (*)(Display *, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XNextEvent");
	if (!x11.XNextEvent)
		goto err;
	x11.XPeekEvent =
	  (int (*)(Display *, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XPeekEvent");
	if (!x11.XPeekEvent)
		goto err;
	x11.XWindowEvent =
	  (int (*)(Display *, Window, long, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XWindowEvent");
	if (!x11.XWindowEvent)
		goto err;
	x11.XMaskEvent =
	  (int (*)(Display *, long, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XMaskEvent");
	if (!x11.XMaskEvent)
		goto err;
	x11.XCheckWindowEvent =
	  (Bool (*)(Display *, Window, long, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XCheckWindowEvent");
	if (!x11.XCheckWindowEvent)
		goto err;
	x11.XCheckMaskEvent =
	  (Bool (*)(Display *, long, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XCheckMaskEvent");
	if (!x11.XCheckMaskEvent)
		goto err;
	x11.XCheckTypedEvent =
	  (Bool (*)(Display *, long, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XCheckTypedEvent");
	if (!x11.XCheckTypedEvent)
		goto err;
	x11.XCheckTypedWindowEvent =
	  (Bool (*)(Display *, Window, int, XEvent *))
	    lib.dlsym(x11.libX11_handle, "XCheckTypedWindowEvent");
	if (!x11.XCheckTypedWindowEvent)
		goto err;
	x11.XIfEvent =
	  (int (*)(Display *, XEvent *, Bool (*)(), XPointer))
	    lib.dlsym(x11.libX11_handle, "XIfEvent");
	if (!x11.XIfEvent)
		goto err;
	x11.XCheckIfEvent =
	  (Bool (*)(Display *, XEvent *, Bool (*)(), XPointer))
	    lib.dlsym(x11.libX11_handle, "XCheckIfEvent");
	if (!x11.XCheckIfEvent)
		goto err;
	x11.XPeekIfEvent =
	  (int (*)(Display *, XEvent *, Bool (*)(), XPointer))
	    lib.dlsym(x11.libX11_handle, "XPeekIfEvent");
	if (!x11.XPeekIfEvent)
		goto err;

	x11.XF86VidModeSetGamma = NULL;
	x11.libXxf86vm_handle = lib.dlopen("libXxf86vm.so", RTLD_LAZY);
	if (x11.libXxf86vm_handle) {
		x11.XF86VidModeSetGamma =
		  (Bool (*)(Display *, int, XF86VidModeGamma *))
		    lib.dlsym(x11.libX11_handle, "XF86VidModeGamma");
	}

	return;
err:
	fprintf(stderr, "(glc:x11) can't get real X11\n");
	exit(1);
}

__PUBLIC int XNextEvent(Display *display, XEvent *event_return)
{
	return __x11_XNextEvent(display, event_return);
}

int __x11_XNextEvent(Display *display, XEvent *event_return)
{
	INIT_GLC
	int ret = x11.XNextEvent(display, event_return);
	x11_event(display, event_return);
	return ret;
}

__PUBLIC int XPeekEvent(Display *display, XEvent *event_return)
{
	return __x11_XPeekEvent(display, event_return);
}

int __x11_XPeekEvent(Display *display, XEvent *event_return)
{
	INIT_GLC
	int ret = x11.XPeekEvent(display, event_return);
	x11_event(display, event_return);
	return ret;
}

__PUBLIC int XWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return)
{
	return __x11_XWindowEvent(display, w, event_mask, event_return);
}

int __x11_XWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return)
{
	INIT_GLC
	int ret = x11.XWindowEvent(display, w, event_mask, event_return);
	x11_event(display, event_return);
	return ret;
}

__PUBLIC Bool XCheckWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return)
{
	return __x11_XCheckWindowEvent(display, w, event_mask, event_return);
}

Bool __x11_XCheckWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return)
{
	Bool ret = x11.XCheckWindowEvent(display, w, event_mask, event_return);
	if (ret)
		x11_event(display, event_return);
	return ret;
}

__PUBLIC int XMaskEvent(Display *display, long event_mask, XEvent *event_return)
{
	return __x11_XMaskEvent(display, event_mask, event_return);
}

int __x11_XMaskEvent(Display *display, long event_mask, XEvent *event_return)
{
	INIT_GLC
	int ret = x11.XMaskEvent(display, event_mask, event_return);
	x11_event(display, event_return);
	return ret;
}

__PUBLIC Bool XCheckMaskEvent(Display *display, long event_mask, XEvent *event_return)
{
	return __x11_XCheckMaskEvent(display, event_mask, event_return);
}

Bool __x11_XCheckMaskEvent(Display *display, long event_mask, XEvent *event_return)
{
	Bool ret = x11.XCheckMaskEvent(display, event_mask, event_return);
	if (ret)
		x11_event(display, event_return);
	return ret;
}

__PUBLIC Bool XCheckTypedEvent(Display *display, int event_type, XEvent *event_return)
{
	return __x11_XCheckTypedEvent(display, event_type, event_return);
}

Bool __x11_XCheckTypedEvent(Display *display, int event_type, XEvent *event_return)
{
	Bool ret = x11.XCheckTypedEvent(display, event_type, event_return);
	if (ret)
		x11_event(display, event_return);
	return ret;
}

__PUBLIC Bool XCheckTypedWindowEvent(Display *display, Window w, int event_type, XEvent *event_return)
{
	return __x11_XCheckTypedWindowEvent(display, w, event_type, event_return);
}

Bool __x11_XCheckTypedWindowEvent(Display *display, Window w, int event_type, XEvent *event_return)
{
	Bool ret = x11.XCheckTypedWindowEvent(display, w, event_type, event_return);
	if (ret)
		x11_event(display, event_return);
	return ret;
}

__PUBLIC int XIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg)
{
	return __x11_XIfEvent(display, event_return, predicate, arg);
}

int __x11_XIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg)
{
	INIT_GLC
	int ret = x11.XIfEvent(display, event_return, predicate, arg);
	x11_event(display, event_return);
	return ret;
}

__PUBLIC Bool XCheckIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg)
{
	return __x11_XCheckIfEvent(display, event_return, predicate, arg);
}

Bool __x11_XCheckIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg)
{
	Bool ret = x11.XCheckIfEvent(display, event_return, predicate, arg);
	if (ret)
		x11_event(display, event_return);
	return ret;
}

__PUBLIC int XPeekIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg)
{
	return __x11_XPeekIfEvent(display, event_return, predicate, arg);
}

int __x11_XPeekIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg)
{
	INIT_GLC
	int ret = x11.XPeekIfEvent(display, event_return, predicate, arg);
	x11_event(display, event_return);
	return ret;
}

__PUBLIC Bool XF86VidModeSetGamma(Display *display, int screen, XF86VidModeGamma *Gamma)
{
	return __x11_XF86VidModeSetGamma(display, screen, Gamma);
}

Bool __x11_XF86VidModeSetGamma(Display *display, int screen, XF86VidModeGamma *Gamma)
{
	INIT_GLC

	if (x11.XF86VidModeSetGamma == NULL)
		return False; /* might not be present */

	Bool ret = x11.XF86VidModeSetGamma(display, screen, Gamma);
	gl_capture_refresh_color(lib.gl);

	return ret;
}

/**  \} */
/**  \} */