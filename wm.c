/* wm - X11 Window Manager */
/* Based on tinywm-xcb.  */
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
enum KeyCodeEnum {
  Tab=23, Left=43, Down=44, Up=45, Right=46, Enter=36

};
int main(int const ArgumentsCount, char const ** Arguments) {
  /* Declare variables.  */
  uint32_t Values[5];
  xcb_connection_t * X;
  xcb_gcontext_t GraphicsContext;
  xcb_generic_event_t * Event;
  xcb_get_geometry_reply_t *Geometry;
  xcb_key_press_event_t * KeyPress;
  xcb_button_press_event_t * ButtonPress;
  xcb_query_pointer_reply_t * Pointer;
  xcb_setup_t const * Setup;
  xcb_screen_iterator_t ScreenIterator;
  xcb_screen_t * Screen;
  xcb_void_cookie_t Cookie;
  xcb_window_t Root;
  xcb_window_t Window;
  /* Initialize variables.  */
  /* Open connection.  */
  X = xcb_connect(NULL,NULL);
  if (xcb_connection_has_error(X)) {
    fputs("DISPLAY\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }
  /* Generate IDs.  */
  GraphicsContext = xcb_generate_id(X);
  /* Get screen information.  */
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
  /* Grab events.   */
  Values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
  Cookie = xcb_change_window_attributes_checked(X, Root,
    XCB_CW_EVENT_MASK, Values);
  if (xcb_request_check(X, Cookie)) {
    /* Another window manager is running.  */
    fputs("OCCUPIED\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }
  /* Grab buttons.  */
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 1, XCB_MOD_MASK_1);
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 3, XCB_MOD_MASK_1);
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, 1, XCB_NONE);
  /* Grab keys.  */
  Cookie = xcb_grab_key(X, 1, Root, XCB_MOD_MASK_CONTROL |
    XCB_MOD_MASK_1, XCB_GRAB_ANY,
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_flush(X);
  /* Process events.  */
  for (;;) {
    Event = xcb_wait_for_event(X);
    switch (Event->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
      fputs("BUTTON\n", stderr);
      ButtonPress = (xcb_button_press_event_t*)Event;
      switch (ButtonPress->detail) {
      case 0:
        fputs("0\n", stderr);
        break;
      case 1:
        fputs("1\n", stderr);
        break;
      case 2:
        fputs("2\n", stderr);
        break;
      }
      break;
    case XCB_CLIENT_MESSAGE:
      fputs("CLIENT\n", stderr);
      break;
    case XCB_CONFIGURE_NOTIFY:
      fputs("CONFIGURE\n", stderr);
      break;
    case XCB_KEY_PRESS:
      KeyPress = (xcb_key_press_event_t*)Event;
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
        default:
          fprintf(stderr, "KEY %d", (int)KeyPress->detail);
      }
      break;
    case XCB_MOTION_NOTIFY:
      fputs("MOTION\n", stderr);
      Pointer = xcb_query_pointer_reply(X, xcb_query_pointer(X, Root), 0);
      Geometry = xcb_get_geometry_reply(X, xcb_get_geometry(X, Window), NULL);
    default:
      fputs("EVENT\n", stderr);
    }
    free(Event);
  }
  xcb_disconnect(X);
  return 0;
}
