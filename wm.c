/* wm - X11 Window Manager */
/* Based on tinywm-xcb. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
/* Begin configuration.  */
#define WM_TERMINAL_COMMAND "xterm"
#define WM_LOCK_COMMAND "slock"
/* Use  Ctrl + Alt as the hot key. */
#define WM_MOD_MASK (XCB_MOD_MASK_1|XCB_MOD_MASK_CONTROL)
/* Uncomment the following to use Alt as the hot key.  */
/*  #define WM_MOD_MASK XCB_MOD_MASK_1 */
/* Uncomment the following to use Super (Windows) as the hot key.  */
/* #define WM_MOD_MASK XCB_MOD_MASK_4 */
/* Show the key codes.  */
//#define WM_DEBUG_XCB_KEY_PRESS
/* End Configuration */

enum KeyCodeEnum {
  EscapeKey=9, TabKey=23, QKey=24, PKey=33,EnterKey=36, HKey=43, JKey=44,
  KKey=45, LKey=46, ZKey=52, XKey=53, SpaceKey=65, UpKey=111, DownKey=116
};

static xcb_atom_t getAtom(xcb_connection_t * X, char const * Name) {
  xcb_atom_t Value;
  xcb_intern_atom_cookie_t Cookie;
  xcb_intern_atom_reply_t *Reply;
  uint8_t Length;
  for (Length = 0; Name[Length]; ++Length)
    ;
  Cookie = xcb_intern_atom(X, false, Length, Name);
  Reply = xcb_intern_atom_reply(X, Cookie, NULL);
  Value = Reply->atom;
  free(Reply);
  return Value;
}

static void modifyRelatively(xcb_connection_t * X,
  xcb_window_t const Window, int8_t const DeltaX, int8_t const DeltaY,
  bool const IsResizing) {
  xcb_get_geometry_reply_t * Reply;
  xcb_get_geometry_cookie_t Cookie;
  uint32_t Values[4];
  uint32_t Mask;
  Cookie = xcb_get_geometry(X, Window);
  Reply = xcb_get_geometry_reply(X, Cookie, NULL);
  if (IsResizing) {
    Mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    Values[0] = Reply->width + DeltaX;
    Values[1] = Reply->height + DeltaY;
  } else {
    Mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
    Values[0] = Reply->x + DeltaX;
    Values[1] = Reply->y + DeltaY;
  }
  free(Reply);
  xcb_configure_window(X, Window, Mask, Values);
  // Prevent a window below this one from getting focus.
  xcb_warp_pointer(X, XCB_NONE, Window, 0,0,0,0,0,0);
}

static void deleteWindow(xcb_connection_t * X, xcb_window_t const Window) {
  xcb_client_message_event_t Message;
  xcb_atom_t const DeleteAtom = getAtom(X, "WM_DELETE_WINDOW");
  xcb_atom_t const ProtocolsAtom = getAtom(X, "WM_PROTOCOLS");
  Message.data.data32[0]=DeleteAtom;
  Message.data.data32[1]=XCB_CURRENT_TIME;
  Message.format=32;
  Message.response_type = XCB_CLIENT_MESSAGE;
  Message.sequence = 0;
  Message.type=ProtocolsAtom;
  Message.window=Window;
  xcb_send_event(X, false, Window, XCB_EVENT_MASK_NO_EVENT, (char*)&Message);
  xcb_flush(X);
  //#define WM_DEBUG_KILLWINDOW
#ifdef WM_DEBUG_KILLWINDOW
  fprintf(stderr, "ProtocolsAtom:%d, DeleteAtom:%d, Window:%d\n",
    ProtocolsAtom, DeleteAtom, Window);
#endif // WM_DEBUG_KILLWINDOW
}

static void grabButton(xcb_connection_t * X, xcb_window_t const Root,
  uint8_t const Button) {
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, Button, WM_MOD_MASK);
  /* Grab button if Num Lock is on.  */
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, Button, WM_MOD_MASK |
    XCB_MOD_MASK_2);
  /* Grab button if Caps Lock is on.  */
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, Button, WM_MOD_MASK |
    XCB_MOD_MASK_LOCK);
  /* Grab button if Num Lock and Caps Lock are on.  */
  xcb_grab_button(X, 0, Root, XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC, Root, XCB_NONE, Button, WM_MOD_MASK |
    XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK);
}

static void inductWindow(xcb_connection_t * X, xcb_window_t const Window) {
  uint32_t Value;
  /* Grab events.  */
  Value = XCB_EVENT_MASK_ENTER_WINDOW;
  xcb_change_window_attributes(X, Window, XCB_CW_EVENT_MASK, &Value);
  xcb_map_window(X, Window);
}

static void processExisting(xcb_connection_t * X,
  xcb_query_tree_cookie_t const Cookie) {
  /* Get existing windows, listen for enter event for sloppy focus.  */
  {
    xcb_query_tree_reply_t * Query;
    xcb_window_t * Children;
    int Length;
    uint32_t Value;
    Query = xcb_query_tree_reply(X, Cookie, NULL);
    Children = xcb_query_tree_children(Query);
    Length = xcb_query_tree_children_length(Query);
    Value = XCB_EVENT_MASK_ENTER_WINDOW;
    while (Length--) {
      xcb_change_window_attributes(X, Children[Length], XCB_CW_EVENT_MASK,
        &Value);
#ifdef WM_DEBUG_XCB_QUERY_TREE
      fprintf(stderr, "%d ", Children[Length]);
#endif /* WM_DEBUG_XCB_QUERY_TREE */
    }
#ifdef WM_DEBUG_XCB_QUERY_TREE
    fprintf(stderr, "\n");
#endif /* WM_DEBUG_XCB_QUERY_TREE */
    free(Query);
  }

}

static inline void stack(xcb_connection_t * X, xcb_window_t const Window,
  uint32_t const Value) {
  xcb_configure_window(X, Window, XCB_CONFIG_WINDOW_STACK_MODE, &Value);
}

static void drag(xcb_connection_t * X, int16_t * Start, xcb_window_t const Root,
  xcb_window_t const Window, bool const IsResizing) {
  xcb_get_geometry_cookie_t GeometryCookie;
  xcb_get_geometry_reply_t *Geometry;
  GeometryCookie = xcb_get_geometry(X, Window);
  stack(X, Window, XCB_STACK_MODE_ABOVE);
  xcb_grab_pointer(X, 0, Root, XCB_EVENT_MASK_BUTTON_MOTION |
    XCB_EVENT_MASK_BUTTON_RELEASE,
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, Root, XCB_NONE,
    XCB_CURRENT_TIME);
  Geometry = xcb_get_geometry_reply(X, GeometryCookie, NULL);
  if (IsResizing) {
    xcb_warp_pointer(X, XCB_NONE, Window, 0,0,0,0, Geometry->width-1,
      Geometry->height-1);
    Start[0] = Geometry->x;
    Start[1] = Geometry->y;
  } else {
    Start[0]-=Geometry->x;
    Start[1]-=Geometry->y;
  }
  free (Geometry);
}

static xcb_screen_t * getScreen(xcb_connection_t * X) {
  xcb_screen_iterator_t ScreenIterator;
  xcb_setup_t const * Setup;
  /* Get screen information. */
  Setup = xcb_get_setup(X);
  ScreenIterator = xcb_setup_roots_iterator(Setup);
  return ScreenIterator.data;
}

static inline xcb_window_t getRoot(xcb_connection_t * X) {
  xcb_screen_t * Screen;
  Screen = getScreen(X);
  return Screen->root;
}

static void setEventMask(xcb_connection_t * X, xcb_window_t const Root) {
  xcb_void_cookie_t Cookie;
  uint32_t Value;
  Value = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
  Cookie=xcb_change_window_attributes_checked(X, Root, XCB_CW_EVENT_MASK,
    &Value);
  /* Determine if another window manager is running.  */
  if (xcb_request_check(X, Cookie)) {
    /* Another window manager is running. */
    fputs("OCCUPIED\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }
}

static xcb_window_t handleConfigureRequest(xcb_connection_t * X,
  xcb_generic_event_t * Event) {
  uint32_t Values[4];
  xcb_window_t Window;
  xcb_configure_request_event_t * Configure;
  Configure = (xcb_configure_request_event_t *)Event;
  Window = Configure->window;
  /* #define WM_DEBUG_XCB_CONFIGURE_REQUEST */
#ifdef WM_DEBUG_XCB_CONFIGURE_REQUEST
  fprintf(stderr, "XCB_CONFIGURE_REQUEST %d %d %d %d\n",
    Configure->x, Configure->y, Configure->width, Configure->height);
#endif /* WM_DEBUG_XCB_CONFIGURE_REQUEST */
  Values[0] = Configure->x;
  Values[1] = Configure->y;
  Values[2] = Configure->width;
  Values[3] = Configure->height;
  xcb_configure_window(X, Window, XCB_CONFIG_WINDOW_WIDTH |
    XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
    Values);
  inductWindow(X, Window);
  return Window;
}

static xcb_window_t handleButtonPress(xcb_connection_t * X,
  xcb_generic_event_t * Event, short Start[2], bool * IsResizing) {
  xcb_button_press_event_t * ButtonPress;
  xcb_window_t Root;
  xcb_window_t Window;
  ButtonPress = (xcb_button_press_event_t*)Event;
  /* #define WM_DEBUG_XCB_BUTTON_PRESS */
#ifdef WM_DEBUG_XCB_BUTTON_PRESS
  fprintf(stderr, "%x %x %x %x %x\n", ButtonPress->event,
    ButtonPress->child, ButtonPress->root, ButtonPress->state,
    ButtonPress->detail);
#endif /* WM_DEBUG_XCB_BUTTON_PRESS */
  Root = ButtonPress->root;
  Window = ButtonPress->child;
  xcb_set_input_focus(X, XCB_INPUT_FOCUS_POINTER_ROOT, Window,
    XCB_CURRENT_TIME);
  if (Window) {
    switch (ButtonPress->detail) {
    case 1: /* move */
      Start[0] = ButtonPress->event_x;
      Start[1] = ButtonPress->event_y;
      *IsResizing = false;
      drag(X, Start, Root, Window, false);
      break;
    case 2: /* lower */
      stack(X, Window, XCB_STACK_MODE_BELOW);
      break;
    case 3: /* resize */
      *IsResizing = true;
      drag(X, Start, Root, Window, true);
      break;
    }
  }
  return Window;
}

static void handleMotionNotify(xcb_connection_t * X,
  xcb_generic_event_t * Event, short Start[2],
  xcb_window_t const Window, bool const IsResizing){
  xcb_motion_notify_event_t * Motion;
  uint32_t Values[2];
  Motion = (xcb_motion_notify_event_t *)Event;
  /* #define WM_DEBUG_XCB_MOTION_NOTIFY */
#ifdef WM_DEBUG_XCB_MOTION_NOTIFY
  fprintf(stderr, "Window: %d, IsResizing:%d, Values[0]:%d, Values[1]:%d\n",
    Window, IsResizing, Values[0], Values[1]);
#endif /* WM_DEBUG_XCB_MOTION_NOTIFY */
  Values[0] = Motion->event_x-Start[0];
  Values[1] = Motion->event_y-Start[1];
  xcb_configure_window(X, Window,
    IsResizing ? XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
    : XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, Values);
}

static xcb_window_t handleEnterNotify(xcb_connection_t * X,
  xcb_generic_event_t * Event) {
  xcb_enter_notify_event_t * Enter;
  xcb_window_t Window;
  Enter = (xcb_enter_notify_event_t *)Event;
  Window = Enter->event;
  xcb_set_input_focus(X, XCB_INPUT_FOCUS_POINTER_ROOT, Window,
    XCB_CURRENT_TIME);
  xcb_flush(X);
  return Window;
}

static bool getIsViewable(xcb_connection_t * X, xcb_window_t const Window) {
  xcb_get_window_attributes_reply_t * AttributesReply;
  xcb_get_window_attributes_cookie_t AttributesCookie;
  bool Value;
  AttributesCookie = xcb_get_window_attributes(X, Window);
  AttributesReply = xcb_get_window_attributes_reply(X, AttributesCookie,
    NULL);
  if (AttributesReply) {
    Value = AttributesReply->map_state == XCB_MAP_STATE_VIEWABLE;
    free(AttributesReply);
  } else
    Value = false;
  return Value;
}

static xcb_window_t goToNextWindow(xcb_connection_t * X,
  xcb_window_t const Root, xcb_window_t CurrentWindow) {
  xcb_query_tree_cookie_t QueryCookie;
  xcb_query_tree_reply_t * QueryReply;
  xcb_window_t * Children;
  xcb_window_t Window;
  int Length;
  int I;
  QueryCookie = xcb_query_tree(X, Root);
  QueryReply = xcb_query_tree_reply(X, QueryCookie, NULL);
  Children = xcb_query_tree_children(QueryReply);
  Length = xcb_query_tree_children_length(QueryReply);
  /* Find the current window's index. */
  for (I = Length; I && Children[I] != CurrentWindow; --I)
    ;
  for (;;) {
    /* Select the next window, cycling to first if necessary.  */
    Window = Children[I++ < Length ? I : (I=0)];
    if (Window != CurrentWindow) {
      if(getIsViewable(X, Window)) {
        stack(X, Window, XCB_STACK_MODE_ABOVE);
        xcb_warp_pointer(X, XCB_NONE, Window, 0, 0, 0, 0, 0, 0);
        //#define WM_DEBUG_GOTONEXTWINDOW
#ifdef WM_DEBUG_GOTONEXTWINDOW
        fprintf(stderr, "CurrentWindow:%d, Window: %d, Root: %d, I: %d\n",
          CurrentWindow, Window, Root, I);
#endif // WM_DEBUG_GOTONEXTWINDOW
        break;
      }
    }
  }
  free(QueryReply);
  return Window;
}

/* Make the window dimensions match the screen dimensions, and set the window
 * position to 0, 0.  Specify XCB_CONFIG_WINDOW_* in Mask.  */
static inline void maximizeWindow(xcb_connection_t * X,
  xcb_window_t const Window, uint32_t Mask) {
  xcb_screen_t * Screen;
  uint8_t Index;
  uint32_t Values[5];
  Screen = getScreen(X);
  Index = 0;
  if (Mask & XCB_CONFIG_WINDOW_X)
    Values[Index++] = 0;
  if (Mask & XCB_CONFIG_WINDOW_Y)
    Values[Index++] = 0;
  if (Mask & XCB_CONFIG_WINDOW_WIDTH)
    Values[Index++] = Screen->width_in_pixels;
  if (Mask & XCB_CONFIG_WINDOW_HEIGHT)
    Values[Index++] = Screen->height_in_pixels;
  Values[Index++] = XCB_STACK_MODE_ABOVE;
  Mask |= XCB_CONFIG_WINDOW_STACK_MODE;
  xcb_configure_window(X, Window, Mask, &Values);
}
/* Window is needed as a parameter because KeyPress->event is always
 * the value of the root window.  Returning window allows the current
 * window to be changed, specifically when doing tab window switching.  */
static xcb_window_t handleKeyPress(xcb_connection_t * X,
  xcb_generic_event_t * Event, xcb_window_t Window) {
  xcb_key_press_event_t * KeyPress;
  KeyPress = (xcb_key_press_event_t*)Event;
  //#define WM_DEBUG_XCB_KEY_PRESS
#ifdef WM_DEBUG_XCB_KEY_PRESS
  fprintf(stderr, "detail:%d, state:%d\n", (int)KeyPress->detail,
    KeyPress->state);
#endif /* WM_DEBUG_XCB_KEY_PRESS */
  enum { MoveBy = 10 };
  switch(KeyPress->detail) {
  case DownKey:
    stack(X, Window, XCB_STACK_MODE_BELOW);
    break;
  case EnterKey:
    system(WM_TERMINAL_COMMAND "&");
    break;
  case EscapeKey:
    exit(0);
    break;
  case HKey:
    modifyRelatively(X, Window, -MoveBy, 0, KeyPress->state &
      XCB_KEY_BUT_MASK_SHIFT);
    break;
  case JKey:
    modifyRelatively(X, Window, 0, MoveBy, KeyPress->state &
      XCB_KEY_BUT_MASK_SHIFT);
    break;
  case KKey:
    modifyRelatively(X, Window, 0, -MoveBy, KeyPress->state &
      XCB_KEY_BUT_MASK_SHIFT);
    break;
  case LKey:
    modifyRelatively(X, Window, MoveBy, 0, KeyPress->state &
      XCB_KEY_BUT_MASK_SHIFT);
    break;
  case PKey:
    system(WM_LOCK_COMMAND "&");
    break;
  case QKey:
    deleteWindow(X, Window);
    break;
  case XKey:
    maximizeWindow(X, Window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH);
    break;
  case ZKey:
    maximizeWindow(X, Window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT);
    break;
  case SpaceKey:
    maximizeWindow(X, Window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
      XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT);
    break;
  case TabKey:
    Window = goToNextWindow(X, KeyPress->root, Window);
    break;
  case UpKey:
    stack(X, Window, XCB_STACK_MODE_ABOVE);
    break;
  }
  return Window;
}

static xcb_window_t handleMapRequest(xcb_connection_t * X,
  xcb_generic_event_t * Event) {
  xcb_map_request_event_t * Map;
  xcb_window_t Window;
  /* #define WM_DEBUG_XCB_MAP_REQUEST */
#ifdef WM_DEBUG_XCB_MAP_REQUEST
  fprintf(stderr, "XCB_MAP_REQUEST\n");
#endif /* WM_DEBUG_XCB_MAP_REQUEST */
  Map = (xcb_map_request_event_t *)Event;
  Window = Map->window;
  inductWindow(X, Window);
  return Window;
}

int main(int const argc __attribute__((unused)),
  char const ** argv __attribute__((unused))) {
  /* This is the offset from the top left corner of the window at which        
   * dragging starts. */
  short Start[2];
  xcb_connection_t * X;
  bool IsResizing = false;
  xcb_query_tree_cookie_t QueryCookie;
  xcb_window_t Root;
  xcb_window_t Window;

  /* Open connection. */
  X = xcb_connect(NULL,NULL);
  if (xcb_connection_has_error(X)) {
    fputs("DISPLAY\n", stderr);
    xcb_disconnect(X);
    exit(1);
  }
  Root = getRoot(X);
  /* Initialize Window.  */
  Window = Root;
  QueryCookie = xcb_query_tree(X, Root);
  setEventMask(X, Root);

  processExisting(X, QueryCookie);

  /* Grab buttons. */
  grabButton(X, Root, 1);
  grabButton(X, Root, 2);
  grabButton(X, Root, 3);

  /* Grab keys. */
  xcb_grab_key(X, 1, Root, WM_MOD_MASK, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC,
    XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(X, 1, Root, WM_MOD_MASK | XCB_MOD_MASK_2, XCB_GRAB_ANY, 
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(X, 1, Root, WM_MOD_MASK | XCB_MOD_MASK_SHIFT, XCB_GRAB_ANY,
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(X, 1, Root, WM_MOD_MASK | XCB_MOD_MASK_LOCK, XCB_GRAB_ANY,
    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(X, 1, Root, WM_MOD_MASK | XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_2,
    XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(X, 1, Root, WM_MOD_MASK | XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_2 |
    XCB_MOD_MASK_LOCK, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(X, 1, Root, WM_MOD_MASK | XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK,
    XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_flush(X);


  /* Process events. */
  for (;;) {
    xcb_generic_event_t * Event;
    Event = xcb_wait_for_event(X);
    switch (Event->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
      Window = handleButtonPress(X, Event, Start, &IsResizing);
      break;
    case XCB_BUTTON_RELEASE:
      xcb_ungrab_pointer(X, XCB_CURRENT_TIME);
      xcb_flush(X);
      break;
    case XCB_ENTER_NOTIFY:
      Window = handleEnterNotify(X, Event);
      break;
    case XCB_KEY_PRESS:
      Window = handleKeyPress(X, Event, Window);
      break;
    case XCB_CONFIGURE_REQUEST:
      Window = handleConfigureRequest(X, Event);
      break;
    case XCB_MAP_REQUEST:
      Window = handleMapRequest(X, Event);
      break;
    case XCB_MOTION_NOTIFY:
      handleMotionNotify(X, Event, Start, Window, IsResizing);
      break;
    }
    xcb_flush(X);
    free(Event);
  }
  xcb_disconnect(X);
  return 0;
}
