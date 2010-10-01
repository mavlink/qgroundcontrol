
/* Copyright (c) Mark J. Kilgard, 1994, 1997, 1998. */
/* Copyright (c) Nate Robins, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* This file completely re-implements glut_menu.c and glut_menu2.c
   for Win32.  Note that neither glut_menu.c nor glut_menu2.c are
   compiled into Win32 GLUT. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "glutint.h"

void (GLUTCALLBACK *__glutMenuStatusFunc) (int, int, int);
GLUTmenu *__glutMappedMenu;
GLUTwindow *__glutMenuWindow;
GLUTmenuItem *__glutItemSelected;
unsigned __glutMenuButton;

static GLUTmenu **menuList = NULL;
static int menuListSize = 0;
static UINT uniqueMenuHandler = 1;

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

void
__glutSetMenu(GLUTmenu * menu)
{
  __glutCurrentMenu = menu;
}

static void
unmapMenu(GLUTmenu * menu)
{
  if (menu->cascade) {
    unmapMenu(menu->cascade);
    menu->cascade = NULL;
  }
  menu->anchor = NULL;
  menu->highlighted = NULL;
}

void
__glutFinishMenu(Window win, int x, int y)
{

  unmapMenu(__glutMappedMenu);

  /* XXX Put in a GdiFlush just in case.  Probably unnecessary. -mjk  */
  GdiFlush();

  if (__glutMenuStatusFunc) {
    __glutSetWindow(__glutMenuWindow);
    __glutSetMenu(__glutMappedMenu);

    /* Setting __glutMappedMenu to NULL permits operations that
       change menus or destroy the menu window again. */
    __glutMappedMenu = NULL;

    __glutMenuStatusFunc(GLUT_MENU_NOT_IN_USE, x, y);
  }
  /* Setting __glutMappedMenu to NULL permits operations that
     change menus or destroy the menu window again. */
  __glutMappedMenu = NULL;

  /* If an item is selected and it is not a submenu trigger,
     generate menu callback. */
  if (__glutItemSelected && !__glutItemSelected->isTrigger) {
    __glutSetWindow(__glutMenuWindow);
    /* When menu callback is triggered, current menu should be
       set to the callback menu. */
    __glutSetMenu(__glutItemSelected->menu);
    __glutItemSelected->menu->select(__glutItemSelected->value);
  }
  __glutMenuWindow = NULL;
}

static void
mapMenu(GLUTmenu * menu, int x, int y)
{
  TrackPopupMenu(menu->win, TPM_LEFTALIGN |
    (__glutMenuButton == TPM_RIGHTBUTTON) ? 
    TPM_RIGHTBUTTON : TPM_LEFTBUTTON,
    x, y, 0, __glutCurrentWindow->win, NULL);
}

void
__glutStartMenu(GLUTmenu * menu, GLUTwindow * window,
		int x, int y, int x_win, int y_win)
{
  assert(__glutMappedMenu == NULL);
  __glutMappedMenu = menu;
  __glutMenuWindow = window;
  __glutItemSelected = NULL;
  if (__glutMenuStatusFunc) {
    __glutSetMenu(menu);
    __glutSetWindow(window);
    __glutMenuStatusFunc(GLUT_MENU_IN_USE, x_win, y_win);
  }
  mapMenu(menu, x, y);
}

GLUTmenuItem *
__glutGetUniqueMenuItem(GLUTmenu * menu, UINT unique)
{
  GLUTmenuItem *item;
  int i;

  i = menu->num;
  item = menu->list;
  while (item) {
    if (item->unique == unique) {
      return item;
    }
    if (item->isTrigger) {
      GLUTmenuItem *subitem;
      subitem = __glutGetUniqueMenuItem(menuList[item->value], unique);
      if (subitem) {
        return subitem;
      }
    }
    i--;
    item = item->next;
  }
  return NULL;
}

GLUTmenuItem *
__glutGetMenuItem(GLUTmenu * menu, HMENU win, int *which)
{
  GLUTmenuItem *item;
  int i;

  i = menu->num;
  item = menu->list;
  while (item) {
    if (item->win == win) {
      *which = i;
      return item;
    }
    if (item->isTrigger) {
      GLUTmenuItem *subitem;

      subitem = __glutGetMenuItem(menuList[item->value],
        win, which);
      if (subitem) {
        return subitem;
      }
    }
    i--;
    item = item->next;
  }
  return NULL;
}

GLUTmenu *
__glutGetMenu(HMENU win)
{
  GLUTmenu *menu;

  menu = __glutMappedMenu;
  while (menu) {
    if (win == menu->win) {
      return menu;
    }
    menu = menu->cascade;
  }
  return NULL;
}

GLUTmenu *
__glutGetMenuByNum(int menunum)
{
  if (menunum < 1 || menunum > menuListSize) {
    return NULL;
  }
  return menuList[menunum - 1];
}

static int
getUnusedMenuSlot(void)
{
  int i;

  /* Look for allocated, unused slot. */
  for (i = 0; i < menuListSize; i++) {
    if (!menuList[i]) {
      return i;
    }
  }
  /* Allocate a new slot. */
  menuListSize++;
  if (menuList) {
    menuList = (GLUTmenu **)
      realloc(menuList, menuListSize * sizeof(GLUTmenu *));
  } else {
    /* XXX Some realloc's do not correctly perform a malloc
       when asked to perform a realloc on a NULL pointer,
       though the ANSI C library spec requires this. */
    menuList = (GLUTmenu **) malloc(sizeof(GLUTmenu *));
  }
  if (!menuList) {
    __glutFatalError("out of memory.");
  }
  menuList[menuListSize - 1] = NULL;
  return menuListSize - 1;
}

static void
menuModificationError(void)
{
  /* XXX Remove the warning after GLUT 3.0. */
  __glutWarning("The following is a new check for GLUT 3.0; update your code.");
  __glutFatalError("menu manipulation not allowed while menus in use.");
}

int APIENTRY
glutCreateMenu(GLUTselectCB selectFunc)
{
  GLUTmenu *menu;
  int menuid;

  if (__glutMappedMenu) {
    menuModificationError();
  }
  menuid = getUnusedMenuSlot();
  menu = (GLUTmenu *) malloc(sizeof(GLUTmenu));
  if (!menu) {
    __glutFatalError("out of memory.");
  }
  menu->id = menuid;
  menu->num = 0;
  menu->submenus = 0;
  menu->select = selectFunc;
  menu->list = NULL;
  menu->cascade = NULL;
  menu->highlighted = NULL;
  menu->anchor = NULL;
  menu->win = CreatePopupMenu();
  menuList[menuid] = menu;
  __glutSetMenu(menu);
  return menuid + 1;
}

int APIENTRY
__glutCreateMenuWithExit(GLUTselectCB selectFunc, void (__cdecl *exitfunc)(int))
{
  __glutExitFunc = exitfunc;
  return glutCreateMenu(selectFunc);
}

void APIENTRY
glutDestroyMenu(int menunum)
{
  GLUTmenu *menu = __glutGetMenuByNum(menunum);
  GLUTmenuItem *item, *next;

  if (__glutMappedMenu) {
    menuModificationError();
  }
  assert(menu->id == menunum - 1);
  DestroyMenu(menu->win);
  menuList[menunum - 1] = NULL;
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

int APIENTRY
glutGetMenu(void)
{
  if (__glutCurrentMenu) {
    return __glutCurrentMenu->id + 1;
  } else {
    return 0;
  }
}

void APIENTRY
glutSetMenu(int menuid)
{
  GLUTmenu *menu;

  if (menuid < 1 || menuid > menuListSize) {
    __glutWarning("glutSetMenu attempted on bogus menu.");
    return;
  }
  menu = menuList[menuid - 1];
  if (!menu) {
    __glutWarning("glutSetMenu attempted on bogus menu.");
    return;
  }
  __glutSetMenu(menu);
}

static void
setMenuItem(GLUTmenuItem * item, const char *label,
	    int value, Bool isTrigger)
{
  GLUTmenu *menu;

  menu = item->menu;
  item->label = __glutStrdup(label);
  if (!item->label) {
    __glutFatalError("out of memory.");
  }
  item->isTrigger = isTrigger;
  item->len = (int) strlen(label);
  item->value = value;
  item->unique = uniqueMenuHandler++;
  if (isTrigger) {
    AppendMenu(menu->win, MF_POPUP, (UINT)item->win, label);
  } else {
    AppendMenu(menu->win, MF_STRING, item->unique, label);
  }
}

void APIENTRY
glutAddMenuEntry(const char *label, int value)
{
  GLUTmenuItem *entry;

  if (__glutMappedMenu) {
    menuModificationError();
  }
  entry = (GLUTmenuItem *) malloc(sizeof(GLUTmenuItem));
  if (!entry) {
    __glutFatalError("out of memory.");
  }
  entry->menu = __glutCurrentMenu;
  setMenuItem(entry, label, value, FALSE);
  __glutCurrentMenu->num++;
  entry->next = __glutCurrentMenu->list;
  __glutCurrentMenu->list = entry;
}

void APIENTRY
glutAddSubMenu(const char *label, int menu)
{
  GLUTmenuItem *submenu;
  GLUTmenu     *popupmenu;

  if (__glutMappedMenu) {
    menuModificationError();
  }
  submenu = (GLUTmenuItem *) malloc(sizeof(GLUTmenuItem));
  if (!submenu) {
    __glutFatalError("out of memory.");
  }
  __glutCurrentMenu->submenus++;
  submenu->menu = __glutCurrentMenu;
  popupmenu = __glutGetMenuByNum(menu);
  if (popupmenu) {
    submenu->win = popupmenu->win;
  }
  setMenuItem(submenu, label, /* base 0 */ menu - 1, TRUE);
  __glutCurrentMenu->num++;
  submenu->next = __glutCurrentMenu->list;
  __glutCurrentMenu->list = submenu;
}

void APIENTRY
glutChangeToMenuEntry(int num, const char *label, int value)
{
	GLUTmenuItem *item;
	int i;
	
	if (__glutMappedMenu) {
		menuModificationError();
	}
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
			item->label = strdup(label);
			if (!item->label) __glutFatalError("out of memory");
			item->isTrigger = FALSE;
			item->len = (int) strlen(label);
			item->value = value;
			item->unique = uniqueMenuHandler++;
			
			RemoveMenu(__glutCurrentMenu->win, (UINT) i - 1, MF_BYPOSITION);
			InsertMenu(__glutCurrentMenu->win, (UINT) i - 1, MF_BYPOSITION | MFT_STRING, item->unique, label);
			
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
	GLUTmenu *popupmenu;
	GLUTmenuItem *item;
	int i;
	
	if (__glutMappedMenu) {
		menuModificationError();
	}
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
			
			item->label = strdup(label);
			if (!item->label)
				__glutFatalError("out of memory");
			item->isTrigger = TRUE;
			item->len = (int) strlen(label);
			item->value = menu - 1;
			item->unique = uniqueMenuHandler++;
			popupmenu = __glutGetMenuByNum(menu);
			if (popupmenu)
				item->win = popupmenu->win;
			
			RemoveMenu(__glutCurrentMenu->win, (UINT) i - 1, MF_BYPOSITION);
			InsertMenu(__glutCurrentMenu->win, (UINT) i - 1, MF_BYPOSITION | MFT_STRING | MF_POPUP, (UINT)item->win, label);
			
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
	GLUTmenuItem *item, **prev;
	int i;
	
	if (__glutMappedMenu) {
		menuModificationError();
	}
	i = __glutCurrentMenu->num;
	prev = &__glutCurrentMenu->list;
	item = __glutCurrentMenu->list;
	while (item) {
		if (i == num) {
			/* Found the menu item in list to remove. */
			__glutCurrentMenu->num--;
			
			/* Patch up menu's item list. */
			*prev = item->next;
			
			RemoveMenu(__glutCurrentMenu->win, (UINT) i - 1, MF_BYPOSITION);
			
			free(item->label);
			free(item);
			return;
		}
		i--;
		prev = &item->next;
		item = item->next;
	}
	__glutWarning("Current menu has no %d item.", num);
}

void APIENTRY
glutAttachMenu(int button)
{
  if (__glutCurrentWindow == __glutGameModeWindow) {
    __glutWarning("cannot attach menus in game mode.");
    return;
  }
  if (__glutMappedMenu) {
    menuModificationError();
  }
  if (__glutCurrentWindow->menu[button] < 1) {
    __glutCurrentWindow->buttonUses++;
  }
  __glutCurrentWindow->menu[button] = __glutCurrentMenu->id + 1;
}

void APIENTRY
glutDetachMenu(int button)
{
  if (__glutMappedMenu) {
    menuModificationError();
  }
  if (__glutCurrentWindow->menu[button] > 0) {
    __glutCurrentWindow->buttonUses--;
    __glutCurrentWindow->menu[button] = 0;
  }
}

