/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
 * Copyright (C) 2009 Maurus Cuelenaere
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#define lrocklib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "rocklib.h"

/*
 * http://www.lua.org/manual/5.1/manual.html#lua_CFunction
 *
 * In order to communicate properly with Lua, a C function must use the following protocol,
 * which defines the way parameters and results are passed: a C function receives its arguments
 * from Lua in its stack in direct order (the first argument is pushed first). To return values to Lua,
 * a C function just pushes them onto the stack, in direct order (the first result is pushed first),
 * and returns the number of results. Any other value in the stack below the results will be properly
 * discarded by Lua. Like a Lua function, a C function called by Lua can also return many results. 
 */
#define RB_WRAP(M) static int rock_##M(lua_State *L)

RB_WRAP(splash)
{
    int ticks = luaL_checkint(L, 1);
    const char *s = luaL_checkstring(L, 2);
    rb->splash(ticks, s);
    return 0;
}

RB_WRAP(lcd_update)
{
    (void)L;
    rb->lcd_update();
    return 0;
}

RB_WRAP(lcd_clear_display)
{
    (void)L;
    rb->lcd_clear_display();
    return 0;
}

RB_WRAP(lcd_putsxy)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    const char* string = luaL_checkstring(L, 3);
    rb->lcd_putsxy(x, y, string);
    return 0;
}

RB_WRAP(lcd_puts)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    const char* string = luaL_checkstring(L, 3);
    rb->lcd_puts(x, y, string);
    return 0;
}

RB_WRAP(lcd_puts_scroll)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    const char* string = luaL_checkstring(L, 3);
    rb->lcd_puts_scroll(x, y, string);
    return 0;
}

RB_WRAP(lcd_stop_scroll)
{
    (void)L;
    rb->lcd_stop_scroll();
    return 0;
}

#ifdef HAVE_LCD_BITMAP
RB_WRAP(lcd_set_drawmode)
{
    int drawmode = luaL_checkint(L, 1);
    rb->lcd_set_drawmode(drawmode);
    return 0;
}

RB_WRAP(lcd_get_drawmode)
{
    int result = rb->lcd_get_drawmode();
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(lcd_setfont)
{
    int font = luaL_checkint(L, 1);
    rb->lcd_setfont(font);
    return 0;
}

RB_WRAP(lcd_drawpixel)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);

    rb->lcd_drawpixel(x, y);
    return 0;
}

RB_WRAP(lcd_drawline)
{
    int x1 = luaL_checkint(L, 1);
    int y1 = luaL_checkint(L, 2);
    int x2 = luaL_checkint(L, 3);
    int y2 = luaL_checkint(L, 4);

    rb->lcd_drawline(x1, y1, x2, y2);
    return 0;
}

RB_WRAP(lcd_hline)
{
    int x1 = luaL_checkint(L, 1);
    int x2 = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);

    rb->lcd_hline(x1, x2, y);
    return 0;
}

RB_WRAP(lcd_vline)
{
    int x = luaL_checkint(L, 1);
    int y1 = luaL_checkint(L, 2);
    int y2 = luaL_checkint(L, 3);

    rb->lcd_vline(x, y1, y2);
    return 0;
}

RB_WRAP(lcd_drawrect)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    int width = luaL_checkint(L, 3);
    int height = luaL_checkint(L, 4);

    rb->lcd_drawrect(x, y, width, height);
    return 0;
}

RB_WRAP(lcd_fillrect)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    int width = luaL_checkint(L, 3);
    int height = luaL_checkint(L, 4);

    rb->lcd_fillrect(x, y, width, height);
    return 0;
}
#endif

RB_WRAP(yield)
{
    (void)L;
    rb->yield();
    return 0;
}

RB_WRAP(sleep)
{
    int ticks = luaL_checkint(L, 1);
    rb->sleep(ticks);
    return 0;
}

RB_WRAP(current_tick)
{
    lua_pushinteger(L, *rb->current_tick);
    return 1;
}

RB_WRAP(button_get)
{
    bool block = lua_toboolean(L, 1);
    long result = rb->button_get(block);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(button_get_w_tmo)
{
    int ticks = luaL_checkint(L, 1);
    long result = rb->button_get_w_tmo(ticks);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(button_status)
{
    int result = rb->button_status();
    lua_pushinteger(L, result);
    return 1;
}

#ifdef HAVE_BUTTON_DATA
RB_WRAP(button_get_data)
{
    int result = rb->button_get_data();
    lua_pushinteger(L, result);
    return 1;
}
#endif

#ifdef HAS_BUTTON_HOLD
RB_WRAP(button_hold)
{
    bool result = rb->button_hold();
    lua_pushboolean(L, result);
    return 1;
}
#endif

RB_WRAP(get_action)
{
    int context = luaL_checkint(L, 1);
    int timeout = luaL_checkint(L, 2);
    int result = rb->get_action(context, timeout);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(action_userabort)
{
    int timeout = luaL_checkint(L, 1);
    bool result = rb->action_userabort(timeout);
    lua_pushboolean(L, result);
    return 1;
}

RB_WRAP(kbd_input)
{
    char* buffer = (char*)luaL_checkstring(L, 1);
    int buflen = luaL_checkint(L, 2);
    int result = rb->kbd_input(buffer, buflen);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(backlight_on)
{
    (void)L;
    rb->backlight_on();
    return 0;
}

RB_WRAP(backlight_off)
{
    (void)L;
    rb->backlight_off();
    return 0;
}

RB_WRAP(backlight_set_timeout)
{
    int val = luaL_checkint(L, 1);
    rb->backlight_set_timeout(val);
    return 0;
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
RB_WRAP(backlight_set_brightness)
{
    int val = luaL_checkint(L, 1);
    rb->backlight_set_brightness(val);
    return 0;
}
#endif

RB_WRAP(open)
{
    const char* pathname = luaL_checkstring(L, 1);
    int flags = luaL_checkint(L, 2);
    int result = rb->open(pathname, flags);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(close)
{
    int fd = luaL_checkint(L, 1);
    int result = rb->close(fd);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(read)
{
    size_t len, n, result = 0;
    luaL_Buffer b;

    int fd = luaL_checkint(L, 1);
    size_t count = luaL_checkint(L, 2);

    luaL_buffinit(L, &b);
    len = LUAL_BUFFERSIZE;
    do
    {
        char *p = luaL_prepbuffer(&b);

        if (len > count)
            len = count;

        n = rb->read(fd, p, len);

        luaL_addsize(&b, n);
        count -= n;
        result += n;
    } while (count > 0 && n == len);
    luaL_pushresult(&b); /* close buffer */

    lua_pushinteger(L, result);
    return 2;
}

RB_WRAP(lseek)
{
    int fd = luaL_checkint(L, 1);
    off_t offset = luaL_checkint(L, 2);
    int whence = luaL_checkint(L, 3);
    off_t result = rb->lseek(fd, offset, whence);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(creat)
{
    const char* pathname = luaL_checkstring(L, 1);
    int result = rb->creat(pathname);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(write)
{
    size_t count;
    int fd = luaL_checkint(L, 1);
    void* buf = (void*)luaL_checklstring(L, 2, &count);
    ssize_t result = rb->write(fd, buf, count);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(remove)
{
    const char* pathname = luaL_checkstring(L, 1);
    int result = rb->remove(pathname);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(rename)
{
    const char* path = luaL_checkstring(L, 1);
    const char* newname = luaL_checkstring(L, 2);
    int result = rb->rename(path, newname);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(ftruncate)
{
    int fd = luaL_checkint(L, 1);
    off_t length = luaL_checkint(L, 2);
    int result = rb->ftruncate(fd, length);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(filesize)
{
    int fd = luaL_checkint(L, 1);
    off_t result = rb->filesize(fd);
    lua_pushinteger(L, result);
    return 1;
}

#define R(NAME) {#NAME, rock_##NAME}
static const luaL_Reg rocklib[] =
{
    /* Graphics */
    R(lcd_clear_display),
    R(lcd_update),
    R(lcd_puts),
    R(lcd_putsxy),
    R(lcd_puts_scroll),
    R(lcd_stop_scroll),
    R(splash),
#ifdef HAVE_LCD_BITMAP
    R(lcd_set_drawmode),
    R(lcd_get_drawmode),
    R(lcd_setfont),
    R(lcd_drawline),
    R(lcd_drawpixel),
    R(lcd_hline),
    R(lcd_vline),
    R(lcd_drawrect),
    R(lcd_fillrect),
#endif

    /* File handling */
    R(open),
    R(close),
    R(read),
    R(lseek),
    R(creat),
    R(write),
    R(remove),
    R(rename),
    R(ftruncate),
    R(filesize),

    /* Kernel */
    R(sleep),
    R(yield),
    R(current_tick),

    /* Buttons */
    R(button_get),
    R(button_get_w_tmo),
    R(button_status),
#ifdef HAVE_BUTTON_DATA
    R(button_get_data),
#endif
#ifdef HAS_BUTTON_HOLD
    R(button_hold),
#endif
    R(get_action),
    R(action_userabort),
    R(kbd_input),

    /* Hardware */
    R(backlight_on),
    R(backlight_off),
    R(backlight_set_timeout),
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    R(backlight_set_brightness),
#endif

    {NULL, NULL}
};
#undef  R

#define RB_CONSTANT(x) lua_pushinteger(L, x); lua_setfield(L, -2, #x);
/*
 ** Open Rockbox library
 */
LUALIB_API int luaopen_rock(lua_State *L)
{
    luaL_register(L, LUA_ROCKLIBNAME, rocklib);

    RB_CONSTANT(HZ);

    RB_CONSTANT(LCD_WIDTH);
    RB_CONSTANT(LCD_HEIGHT);

    RB_CONSTANT(O_RDONLY);
    RB_CONSTANT(O_WRONLY);
    RB_CONSTANT(O_RDWR);
    RB_CONSTANT(O_CREAT);
    RB_CONSTANT(O_APPEND);
    RB_CONSTANT(O_TRUNC);
    RB_CONSTANT(SEEK_SET);
    RB_CONSTANT(SEEK_CUR);
    RB_CONSTANT(SEEK_END);

    return 1;
}

