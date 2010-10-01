
/* Copyright (c) Mark J. Kilgard, 1994, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* glut_menu2.c implements the little used GLUT menu calls in
   a distinct file from glut_menu.c for slim static linking. */

/* The Win32 GLUT file win32_menu.c completely re-implements all
   the menuing functionality implemented.  This file is used only by
   the X Window System version of GLUT. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <X11/Xlib.h>

#include "glutint.h"
#include "layerutil.h"

/* CENTRY */
/* DEPRICATED, use glutMenuStatusFunc instead. */
void APIENTRY 
glutMenuStateFunc(GLUTmenuStateCB menuStateFunc)
{
  __glutMenuStatusFunc = (GLUTmenuStatusCB) menuStateFunc;
}

void APIENTRY 
glutMenuStatusFunc(GLUTmenuStatusCB menuStatusFunc)
{
  __glutMenuStatusFunc = menuStatusFunc;
}

void APIENTRY 
glutDestroyMenu(int menunum)
{
  GLUTmenu *menu = __glutGetMenuByNum(menunum);
  GLUTmenuItem *item, *next;

  if (__glutMappedMenu)
    __glutMenuModificationError();
  assert(menu->id == menunum - 1);
  XDestroySubwindows(__glutDisplay, menu->win);
  XDestroyWindow(__glutDisplay, menu->win);
  __glutMenuList[menunum - 1] = NULL;
  /* free all menu entries */
  item = menu->list;
  while (item) {
    assert(item->menu == menu);
    next = item->next;
    free(item->label);
    free(item);
    item = next;
  }
  if (__glutCurrentMenu == menu) {
    __glutCurrentMenu = NULL;
  }
  free(menu);
}

void APIENTRY 
glutChangeToMenuEntry(int num, const char *label, int value)
{
  GLUTmenuItem *item;
  int i;

  if (__glutMappedMenu)
    __glutMenuModificationError();
  i = __glutCurrentMenu->num;
  item = __glutCurrentMenu->list;
  while (item) {
    if (i == num) {
      if (item->isTrigger) {
        /* If changing a submenu trigger to a menu entry, we
           need to account for submenus.  */
        item->menu->submenus--;
      }
      free(item->label);
      __glutSetMenuItem(item, label, value, False);
      return;
    }
    i--;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

void APIENTRY 
glutChangeToSubMenu(int num, const char *label, int menu)
{
  GLUTmenuItem *item;
  int i;

  if (__glutMappedMenu)
    __glutMenuModificationError();
  i = __glutCurrentMenu->num;
  item = __glutCurrentMenu->list;
  while (item) {
    if (i == num) {
      if (!item->isTrigger) {
        /* If changing a menu entry to as submenu trigger, we
           need to account for submenus.  */
        item->menu->submenus++;
      }
      free(item->label);
      __glutSetMenuItem(item, label, /* base 0 */ menu - 1, True);
      return;
    }
    i--;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

void APIENTRY 
glutRemoveMenuItem(int num)
{
  GLUTmenuItem *item, **prev, *remaining;
  int pixwidth, i;

  if (__glutMappedMenu)
    __glutMenuModificationError();
  i = __glutCurrentMenu->num;
  prev = &__glutCurrentMenu->list;
  item = __glutCurrentMenu->list;
  /* If menu item is removed, the menu's pixwidth may need to
     be recomputed. */
  pixwidth = 1;
  while (item) {
    if (i == num) {
      /* If this menu item's pixwidth is as wide as the menu's
         pixwidth, removing this menu item will necessitate
         shrinking the menu's pixwidth. */
      if (item->pixwidth >= __glutCurrentMenu->pixwidth) {
        /* Continue recalculating menu pixwidth, first skipping
           the removed item. */
        remaining = item->next;
        while (remaining) {
          if (remaining->pixwidth > pixwidth) {
            pixwidth = remaining->pixwidth;
          }
          remaining = remaining->next;
        }
        __glutCurrentMenu->pixwidth = pixwidth;
      }
      __glutCurrentMenu->num--;
      __glutCurrentMenu->managed = False;

      /* Patch up menu's item list. */
      *prev = item->next;

      free(item->label);
      free(item);
      return;
    }
    if (item->pixwidth > pixwidth) {
      pixwidth = item->pixwidth;
    }
    i--;
    prev = &item->next;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

void APIENTRY 
glutDetachMenu(int button)
{
  if (__glutMappedMenu)
    __glutMenuModificationError();
  if (__glutCurrentWindow->menu[button] > 0) {
    __glutCurrentWindow->buttonUses--;
    __glutChangeWindowEventMask(ButtonPressMask | ButtonReleaseMask,
      __glutCurrentWindow->buttonUses > 0);
    __glutCurrentWindow->menu[button] = 0;
  }
}

/* ENDCENTRY */
