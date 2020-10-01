/* wm - X11 Window Manager */
/* Based on tinywm-xcb. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#define WM_TERMINAL_COMMAND "xterm"
/* Uncomment the following to use Alt as the hot key.  */
/*  #define WM_MOD_MASK XCB_MOD_MASK_1 */
/* Uncomment the following to use Super (Windows) as the hot key.  */
#define WM_MOD_MASK XCB_MOD_MASK_4
enum KeyCodeEnum {
  Tab=23, Left=43, Down=44, Up=45, Right=46, Enter=36
};
int main(int const argc __attribute__((unused)),
  char const ** argv __attribute__((unused))) {
  bool IsResizing;
  /* Declare variables. */
  uint32_t Values[5];
  /* This is the offset from the top left corner of the window at which
   * dragging starts. */
  short Start[2];
  uint16_t Mask;
  xcb_connection_t * X;
  xcb_generic_event_t * Event;
  xcb_get_geometry_cookie_t GeometryCookie;
  xcb_get_geometry_reply_t *Geometry;
  xcb_key_press_event_t * KeyPress;
  xcb_button_press_event_t * ButtonPress;
  xcb_motion_notify_event_t * Motion;
  xcb_query_tree_cookie_t QueryCookie;
  xcb_setup_t const * Setup;
  xcb_screen_iterator_t ScreenIterator;
  xcb_screen_t * Screen;
  xcb_void_cookie_t Cookie;
  xcb_window_t Root;
  xcb_window_t Window;
  /* Initialize variables. */
  Geometry = NULL;
  /* Open connection. */
  X = xcb_connect(NULL,NULL);
  if (xcb_connection_has_error(X)) {
    fputs("DISPLAY\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }

  /* Get screen information. */
  Setup = xcb_get_setup(X);
  ScreenIterator = xcb_setup_roots_iterator(Setup);
  Screen = ScreenIterator.data;
  Root = Screen->root;
  QueryCookie = xcb_query_tree(X, Root);

  /* Determine if another window manager is running.  */
  Values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
  Cookie=xcb_change_window_attributes_checked(X, Root, XCB_CW_EVENT_MASK,
    Values);
  if (xcb_request_check(X, Cookie)) {
    /* Another window manager is running. */
    fputs("OCCUPIED\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }

  /* Get existing windows, listen for enter event for sloppy focus.  */
  {
    xcb_query_tree_reply_t * Query;
    xcb_window_t * Children;
    int Length;
    Query = xcb_query_tree_reply(X, QueryCookie, NULL);
    Children = xcb_query_tree_children(Query);
    Length = xcb_query_tree_children_length(Query);
    while (Length--) {
      Values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
      xcb_change_window_attributes(X, Children[Length], XCB_CW_EVENT_MASK,
        Values);
#ifdef WM_DEBUG_XCB_QUERY_TREE
      fprintf(stderr, "%d ", Children[Length]);
#endif /* WM_DEBUG_XCB_QUERY_TREE */
    }
#ifdef WM_DEBUG_XCB_QUERY_TREE
    fprintf(stderr, "\n");
#endif /* WM_DEBUG_XCB_QUERY_TREE */
    free(Query);
  }

  /* Grab buttons. */
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 1, WM_MOD_MASK);
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 2, WM_MOD_MASK);
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 3, WM_MOD_MASK);

  /* Grab keys. */
  xcb_grab_key(X, 1, Root, WM_MOD_MASK, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC);
  xcb_flush(X);

  /* Process events. */
  for (;;) {
    Event = xcb_wait_for_event(X);
    switch (Event->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
      ButtonPress = (xcb_button_press_event_t*)Event;
      /* #define WM_DEBUG_XCB_BUTTON_PRESS */
#ifdef WM_DEBUG_XCB_BUTTON_PRESS
      fprintf(stderr, "%x %x %x %x %x\n", ButtonPress->event,
        ButtonPress->child, ButtonPress->root, ButtonPress->state,
        ButtonPress->detail);
#endif /* WM_DEBUG_XCB_BUTTON_PRESS */
      Window = ButtonPress->child;
      xcb_set_input_focus(X, XCB_INPUT_FOCUS_POINTER_ROOT, Window,
        XCB_CURRENT_TIME);
      if (Window) {
        GeometryCookie = xcb_get_geometry(X, Window);
        switch (ButtonPress->detail) {
        case 1: /* move */
          IsResizing = false;
          Values[0] = XCB_STACK_MODE_ABOVE;
          xcb_configure_window(X, Window, XCB_CONFIG_WINDOW_STACK_MODE,
            Values);
          xcb_grab_pointer(X, 0, Root, XCB_EVENT_MASK_BUTTON_MOTION |
            XCB_EVENT_MASK_BUTTON_RELEASE,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, Root, XCB_NONE,
            XCB_CURRENT_TIME);
          Geometry = xcb_get_geometry_reply(X, GeometryCookie, NULL);
          Start[0] = ButtonPress->event_x-Geometry->x;
          Start[1] = ButtonPress->event_y-Geometry->y;
          free (Geometry);
          Geometry = NULL;
          xcb_flush(X);
          break;
        case 2: /* lower */
          Values[0] = XCB_STACK_MODE_BELOW;
          xcb_configure_window(X, Window, XCB_CONFIG_WINDOW_STACK_MODE, Values);
          xcb_flush(X);
          break;
        case 3: /* resize */
          IsResizing = true;
          Values[0] = XCB_STACK_MODE_ABOVE;
          xcb_configure_window(X, Window, XCB_CONFIG_WINDOW_STACK_MODE, Values);
          xcb_grab_pointer(X, 0, Root, XCB_EVENT_MASK_BUTTON_MOTION |
            XCB_EVENT_MASK_BUTTON_RELEASE,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, Root, XCB_NONE,
            XCB_CURRENT_TIME);
          Geometry = xcb_get_geometry_reply(X, GeometryCookie, NULL);
          xcb_warp_pointer(X, XCB_NONE, Window, 0,0,0,0,
            Geometry->width-1,Geometry->height-1);
          Start[0] = Geometry->x;
          Start[1] = Geometry->y;
          free(Geometry);
          Geometry = NULL;
          xcb_flush(X);
          break;
        }
      }
      break;
    case XCB_BUTTON_RELEASE:
      xcb_ungrab_pointer(X, XCB_CURRENT_TIME);
      xcb_flush(X);
      break;
    case XCB_ENTER_NOTIFY: {
      xcb_enter_notify_event_t * Enter;
      Enter = (xcb_enter_notify_event_t *)Event;
      Window = Enter->event;
      xcb_set_input_focus(X, XCB_INPUT_FOCUS_POINTER_ROOT, Window,
        XCB_CURRENT_TIME);
      xcb_flush(X);
      break;
    }
    case XCB_KEY_PRESS:
      KeyPress = (xcb_key_press_event_t*)Event;
      /* #define WM_DEBUG_XCB_KEY_PRESS */
#ifdef WM_DEBUG_XCB_KEY_PRESS
      fprintf(stderr, "KEY %d\n", (int)KeyPress->detail);
#endif /* WM_DEBUG_XCB_KEY_PRESS */
      switch(KeyPress->detail) {
      case Enter:
        system(WM_TERMINAL_COMMAND "&");
        break;
      }
      break;
    case XCB_CONFIGURE_REQUEST: {
      xcb_configure_request_event_t * Configure;
      Configure = (xcb_configure_request_event_t *)Event;
      Window = Configure->window;
      /* #define WM_DEBUG_XCB_CONFIGURE_REQUEST */
#ifdef WM_DEBUG_XCB_CONFIGURE_REQUEST
      fprintf(stderr, "XCB_CONFIGURE_REQUEST\n");
#endif /* WM_DEBUG_XCB_CONFIGURE_REQUEST */
      Values[0] = Configure->x;
      Values[1] = Configure->y;
      Values[2] = Configure->width;
      Values[3] = Configure->height;
      Mask= XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
      xcb_configure_window(X, Window, Mask, Values);
      /* Grab events.  */
      Values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
      xcb_change_window_attributes(X, Window, XCB_CW_EVENT_MASK, Values);
      xcb_map_window(X, Window);
      xcb_flush(X);
      break;
    }
    case XCB_MOTION_NOTIFY:
      Motion = (xcb_motion_notify_event_t *)Event;
      /* #define WM_DEBUG_XCB_MOTION_NOTIFY */
#ifdef WM_DEBUG_XCB_MOTION_NOTIFY
      fprintf(stderr, "Window: %d, IsResizing:%d, Values[0]:%d, Values[1]:%d\n",
        Window, IsResizing, Values[0], Values[1]);
#endif /* WM_DEBUG_XCB_MOTION_NOTIFY */
      Values[0] = Motion->event_x-Start[0];
      Values[1] = Motion->event_y-Start[1];
      Mask=IsResizing ? XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
        : XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
      xcb_configure_window(X, Window, Mask, Values);
      xcb_flush(X);
      break;
    }
    free(Event);
    if (Geometry) {
      free(Geometry);
      Geometry = NULL;
    }
  }
  xcb_disconnect(X);
  return 0;
}
