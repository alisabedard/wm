/* wm - X11 Window Manager */
/* Based on tinywm-xcb. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
enum KeyCodeEnum {
  Tab=23, Left=43, Down=44, Up=45, Right=46, Enter=36

};
int main(int const ArgumentsCount, char const ** Arguments) {
  bool IsResizing;
  /* Declare variables. */
  uint32_t Values[5];
  /* This is the offset from the top left corner of the window at which
   * dragging starts. */
  short Start[2];
  uint16_t Mask;
  xcb_connection_t * X;
  xcb_gcontext_t GraphicsContext;
  xcb_generic_event_t * Event;
  xcb_get_geometry_cookie_t GeometryCookie;
  xcb_get_geometry_reply_t *Geometry;
  xcb_key_press_event_t * KeyPress;
  xcb_button_press_event_t * ButtonPress;
  xcb_motion_notify_event_t * Motion;
  xcb_query_pointer_reply_t * Pointer;
  xcb_setup_t const * Setup;
  xcb_screen_iterator_t ScreenIterator;
  xcb_screen_t * Screen;
  xcb_void_cookie_t Cookie;
  xcb_window_t Root;
  xcb_window_t Window;
  /* Initialize variables. */
  /* Open connection. */
  X = xcb_connect(NULL,NULL);
  if (xcb_connection_has_error(X)) {
    fputs("DISPLAY\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }

  /* Generate IDs. */
  GraphicsContext = xcb_generate_id(X);

  /* Get screen information. */
  {
    Setup = xcb_get_setup(X);
    ScreenIterator = xcb_setup_roots_iterator(Setup);
  }
  Screen = ScreenIterator.data;
  Root = Screen->root;
  Values[0] = XCB_GX_XOR;
  Values[1] = 1;
  xcb_create_gc(X, GraphicsContext, Root,
    XCB_GC_FUNCTION | XCB_GC_LINE_WIDTH, Values);

  /* Grab events.  */
  Values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
  Cookie = xcb_change_window_attributes_checked(X, Root,
    XCB_CW_EVENT_MASK, Values);
  if (xcb_request_check(X, Cookie)) {
    /* Another window manager is running. */
    fputs("OCCUPIED\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }

  /* Grab buttons. */
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 1, XCB_MOD_MASK_1);
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 2, XCB_MOD_MASK_1);
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 3, XCB_MOD_MASK_1);

  /* Grab keys. */
  Cookie = xcb_grab_key(X, 1, Root, XCB_MOD_MASK_CONTROL |
    XCB_MOD_MASK_1, XCB_GRAB_ANY,
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_flush(X);

  /* Process events. */
  for (;;) {
    Event = xcb_wait_for_event(X);
    switch (Event->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
      ButtonPress = (xcb_button_press_event_t*)Event;
      /* #define DEBUG_XCB_BUTTON_PRESS */
#ifdef DEBUG_XCB_BUTTON_PRESS
      fprintf(stderr, "%x %x %x %x %x\n", ButtonPress->event,
        ButtonPress->child, ButtonPress->root, ButtonPress->state,
        ButtonPress->detail);
#endif /* DEBUG_XCB_BUTTON_PRESS */
      Window = ButtonPress->child;
      if (Window) {
        GeometryCookie = xcb_get_geometry(X, Window);
        switch (ButtonPress->detail) {
        case 1: /* move */
          IsResizing = false;
          Values[0] = XCB_STACK_MODE_ABOVE;
          xcb_configure_window(X, Window, XCB_CONFIG_WINDOW_STACK_MODE, Values);
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
    case XCB_KEY_PRESS:
      KeyPress = (xcb_key_press_event_t*)Event;
#define DEBUG_XCB_KEY_PRESS
#ifdef DEBUG_XCB_KEY_PRESS
      fprintf(stderr, "KEY %d\n", (int)KeyPress->detail);
#endif /* DEBUG_XCB_KEY_PRESS */
#if 0
      switch(KeyPress->detail) {
      case Left:
        break;
      case Down:
        break;
      case Up:
        break;
      case Right:
        break;
      case Enter:
        break;
      case Tab:
        break;
      }
#endif /* 0 */
      break;
    case XCB_MOTION_NOTIFY:
      Motion = (xcb_motion_notify_event_t *)Event;
//#define DEBUG_XCB_MOTION_NOTIFY
#ifdef DEBUG_XCB_MOTION_NOTIFY
      fprintf(stderr, "Window: %d, IsResizing:%d, Values[0]:%d, Values[1]:%d\n",
        Window, IsResizing, Values[0], Values[1]);
#endif /* DEBUG_XCB_MOTION_NOTIFY */
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
