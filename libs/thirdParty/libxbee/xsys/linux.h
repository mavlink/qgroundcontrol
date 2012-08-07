/*
  libxbee - a C library to aid the use of Digi's Series 1 XBee modules
            running in API mode (AP=2).

  Copyright (C) 2009  Attie Grande (attie@attie.co.uk)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  this file contains code that is used by Linux ONLY */
#ifndef __GNUC__
#error "This file should only be used on a Linux system"
#endif

/* ################################################################# */
/* ### Linux Code ################################################## */
/* ################################################################# */

#define xbee_thread_create(a,b,c) pthread_create(&(a),NULL,(void *(*)(void *))(b),(void *)(c))
#define xbee_thread_cancel(a,b)   pthread_cancel((a))
#define xbee_thread_join(a)       pthread_join((a),NULL)
#define xbee_thread_tryjoin(a)    pthread_tryjoin_np((a),NULL)

#define xbee_mutex_init(a)        pthread_mutex_init(&(a),NULL)
#define xbee_mutex_destroy(a)     pthread_mutex_destroy(&(a))
#define xbee_mutex_lock(a)        pthread_mutex_lock(&(a))
#define xbee_mutex_trylock(a)     pthread_mutex_trylock(&(a))
#define xbee_mutex_unlock(a)      pthread_mutex_unlock(&(a))

#define xbee_sem_init(a)          sem_init(&(a),0,0)
#define xbee_sem_destroy(a)       sem_destroy(&(a))
#define xbee_sem_wait(a)          sem_wait(&(a))
#define xbee_sem_post(a)          sem_post(&(a))

#define xbee_cond_init(a)         pthread_cond_init(&(a),NULL)
#define xbee_cond_destroy(a)      pthread_cond_destroy(&(a))
#define xbee_cond_wait(a,b)       pthread_cond_wait(&(a),&(b))
#define xbee_cond_signal(a)       pthread_cond_signal(&(a))
#define xbee_cond_broadcast(a)    pthread_cond_broadcast(&(a))

#define xbee_write(xbee,a,b)      fwrite((a),1,(b),(xbee)->tty)
#define xbee_read(xbee,a,b)       fread((a),1,(b),(xbee)->tty)
#define xbee_ferror(xbee)         ferror((xbee)->tty)
#define xbee_feof(xbee)           feof((xbee)->tty)
#define xbee_close(a)             fclose((a))

