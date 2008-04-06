/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Antoine Cellerier <dionoea -at- videolan -dot- org>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _LIB_BMP_H_
#define _LIB_BMP_H_

#include "lcd.h"
#include "plugin.h"

#ifdef HAVE_LCD_COLOR
/**
 * Save bitmap to file
 */
int save_bmp_file( char* filename, struct bitmap *bm, struct plugin_api* rb  );
#endif

/**
   Very simple image scale from src to dst (nearest neighbour).
   Source and destination dimensions are read from the struct bitmap.
*/
void simple_resize_bitmap(struct bitmap *src, struct bitmap *dst);

/**
   Advanced image scale from src to dst (bilinear) based on imlib2.
   Source and destination dimensions are read from the struct bitmap.
 */
void smooth_resize_bitmap(struct bitmap *src,  struct bitmap *dst);

#endif
