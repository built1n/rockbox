/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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
#ifndef STRING_EXTRA_H
#define STRING_EXTRA_H
#include <string.h>
#include "strlcpy.h"
#include "strlcat.h"
#include "strcasecmp.h"
#include "strcasestr.h"
#include "strtok_r.h"
#include "memset16.h"

#if defined(WIN32) || defined(APPLICATION)
#ifndef mempcpy
#define mempcpy __builtin_mempcpy
#endif
#endif

#endif /* STRING_EXTRA_H */
