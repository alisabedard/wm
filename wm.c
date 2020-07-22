/* wm - X11 Window Manager */
/* Based on jbwm and tinywm-xcb.  */
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
int main(int const ArgumentsCount, char const ** Arguments) {
  /* Declare variables.  */
  int Status;
  xcb_connection_t * X;
  xcb_screen_iterator_t ScreenIterator;
  xcb_screen_t * Screen;
  xcb_gcontext_t GraphicsContext;
  xcb_window_t Root;
  xcb_void_cookie_t Cookie;
  /* Initialize variables.  */
  Status = 0;
  /* Open connection.  */
  X = xcb_connect(NULL,NULL);
  if (xcb_connection_has_error(X)) {
    fputs("DISPLAY\n", stderr);
    Status = 1;
    goto end;
  }
  /* Generate IDs.  */
  GraphicsContext = xcb_generate_id(X);
  /* Get screen information.  */
  {
    xcb_setup_t const * Setup = xcb_get_setup(X);
    ScreenIterator = xcb_setup_roots_iterator(Setup);
  }
  Screen = ScreenIterator.data;
  Root = Screen->root;
  {
    int32_t Values[] = {XCB_GX_XOR, 1};
    xcb_create_gc(X, GraphicsContext, Root,
      XCB_GC_FUNCTION | XCB_GC_LINE_WIDTH, Values);
  }
  /* Grab events.   */
  {
    int32_t Value = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    Cookie = xcb_change_window_attributes_checked(X, Root,
      XCB_CW_EVENT_MASK, &Value);
    if (xcb_request_check(X, Cookie)) {
      /* Another window manager is running.  */
      fputs("OCCUPIED\n", stderr);
      Status = 1;
      goto end;
    }
  }
  /* Grab buttons.  */
  /* Grab keys.  */
  Cookie = xcb_grab_key(X, 1, Root, XCB_MOD_MASK_4, XCB_GRAB_ANY,
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  /* Process events.  */
  for (;;) {
    xcb_generic_event_t * Event;
    Event = xcb_wait_for_event(X);
    switch (Event->response_type & ~0x80) {
    default:
      fputs("EVENT", stderr);
      free(Event);
      goto end;
    }
    free(Event);
  }
  end:
  xcb_disconnect(X);
  return Status;
}
