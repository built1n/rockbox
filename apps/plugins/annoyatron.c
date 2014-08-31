/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "plugin.h"
#include "lib/pluginlib_actions.h"

#define MIN_TIME 1//(60*HZ)
#define MAX_TIME HZ//(120*HZ)

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

enum plugin_status plugin_start(const void* param)
{
    (void)param;
    rb->splash(0, "Press any key to begin...");
    rb->button_get(true);
    rb->srand(*rb->current_tick);
    for(;;)
    {
        long beep_time=*rb->current_tick+(rb->rand()%(MAX_TIME-MIN_TIME)+MIN_TIME);
        rb->splashf(0, "beeping at tick %ld", beep_time);
        while(*rb->current_tick<beep_time)
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            int button=pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));
            if(button==PLA_CANCEL)
                return PLUGIN_OK;
        }
        int beep_ms=rb->rand()%1000+500;
        unsigned int freq=2000;
        int r=rb->rand()%3;
        if(r==0)
            freq=2000;
        else if(r==1)
            freq=12000;
        else
            freq=15000;
        rb->splashf(0, "Freq: %d", freq);
        rb->piezo_play(beep_ms*1000, freq, true);
    }
}
