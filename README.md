# wm - A Window Manager for X
*By Jeffrey E. Bedard*

## Introduction
wm is a window manager for the X11 window system.
wm is a non-reparenting window manager.
wm does not draw anything.
wm has a tiny binary size.
wm consumes very few resources.
wm uses XCB.

## Usage
Add 'exec wm' to the end of your ~/.xinitrc, or start it manually.

## Buttons
Mouse button actions are loosely based on jbwm for familiarity.
- Super-LeftClick *Raise and move the current window.*
- Super-MiddleClick *Lower the current window.*
- Super-RightClick *Raise and resize the current window.*

## Keys
Keyboard commands are loosely based on jbwm for familiarity.
- Super-Down *Lower the current window.*
- Super-Enter *Run xterm.*
- Super-Escape *Quit.*
- Super-Q *Close the window.*
- Super-H *Move window left.*
- Super-J *Move window down.*
- Super-K *Move window up.*
- Super-L *Move window right.*
- Super-X *Maximize horizontally.*
- Super-Z *Maximize vertically.*
- Super-P *Run slock.*
- Super-Space *Maximize the current window.*
- Super-Tab *Switch to the lowest window in the stacking order.*
- Super-Up *Raise the current window.*
