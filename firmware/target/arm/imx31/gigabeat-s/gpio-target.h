/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
 *
 * Gigabeat S GPIO interrupt event descriptions header
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
#ifndef GPIO_TARGET_H
#define GPIO_TARGET_H

/* MC13783 GPIO pin info for this target */
#define MC13783_GPIO_IMR    GPIO1_IMR
#define MC13783_GPIO_NUM    GPIO1_NUM
#define MC13783_GPIO_ISR    GPIO1_ISR
#define MC13783_GPIO_LINE   31

/* SI4700 GPIO STC/RDS pin info for this target */
#define SI4700_GPIO_STC_RDS_IMR     GPIO1_IMR
#define SI4700_GPIO_STC_RDS_NUM     GPIO1_NUM
#define SI4700_GPIO_STC_RDS_ISR     GPIO1_ISR
#define SI4700_GPIO_STC_RDS_LINE    27

#define GPIO1_INT_PRIO      INT_PRIO_DEFAULT

/* Declare event indexes in priority order in a packed array */
enum gpio_event_ids
{
    /* GPIO1 event IDs */
    MC13783_EVENT_ID = GPIO1_EVENT_FIRST,
    SI4700_STC_RDS_EVENT_ID,
    GPIO1_NUM_EVENTS = 2,
    /* GPIO2 event IDs */
    /* none defined */
    /* GPIO3 event IDs */
    /* none defined */
};

void mc13783_event(void);
void si4700_stc_rds_event(void);

#endif /* GPIO_TARGET_H */
