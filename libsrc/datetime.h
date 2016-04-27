/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
 *
 *  Authors: Chet Heilman <cheilman@sicompos.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef DATETIME_H
#define DATETIME_H

#define RLIB_DATETIME_SECSPERDAY (60 * 60 * 24)

void rlib_datetime_clear(struct rlib_datetime *t1);
void rlib_datetime_set_date(struct rlib_datetime *dt, gint y, gint m, gint d);
void rlib_datetime_set_time(struct rlib_datetime *dt, gint h, gint m, gint s);
int rlib_datetime_valid_date(struct rlib_datetime *dt);
gint rlib_datetime_valid_time(struct rlib_datetime *dt);
void rlib_datetime_clear_time(struct rlib_datetime *t);
void rlib_datetime_clear_date(struct rlib_datetime *t);
gint rlib_datetime_compare(struct rlib_datetime *t1, struct rlib_datetime *t2);
gint rlib_datetime_daysdiff(struct rlib_datetime *dt, struct rlib_datetime *dt2);
void rlib_datetime_addto(struct rlib_datetime *dt, gint64 amt);
gint rlib_datetime_secsdiff(struct rlib_datetime *dt, struct rlib_datetime *dt2);
void rlib_datetime_makesamedate(struct rlib_datetime *target, struct rlib_datetime *chgto);
void rlib_datetime_makesametime(struct rlib_datetime *target, struct rlib_datetime *chgto);
long rlib_datetime_time_as_long(struct rlib_datetime *dt);
void rlib_datetime_set_time_from_long(struct rlib_datetime *dt, long t);

#endif

