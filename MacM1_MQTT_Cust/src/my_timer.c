/*
 * my_timer.c
 *
 *  Created on: Oct 25, 2021
 *      Author: Olaf
 */
/*mytimer.c*/

#include <stdint.h>
#include <string.h>
#include <sys/event.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <stdio.h>
#include "my_timer.h"

/**https://qnaplus.com/implement-periodic-timer-linux/*/
/**https://stackoverflow.com/questions/1662909/undefined-reference-to-pthread-create-in-linux*/
/** https://stackoverflow.com/questions/19472546/implicit-declaration-of-function-close */
/*https://www.freebsd.org/cgi/man.cgi?kqueue*/


#define MAX_TIMER_COUNT 1000

struct timer_node
{
    struct kevent             fd;
    time_handler        callback;
    void *              user_data;
    unsigned int        interval;
    struct timer_node * next;
};
void diep(const char *s);
static void * _timer_thread(void * data);
static pthread_t g_thread_id;
struct timer_node  timer ;

struct kevent change;    /* event we want to monitor */
struct kevent event;     /* event that was triggered */


size_t start_timer(unsigned int interval, time_handler handler, void * user_data)
{
    timer.callback  = handler;
    timer.user_data = user_data;
    timer.interval  = interval;
    timer.fd = change;
    if(pthread_create(&g_thread_id, NULL, _timer_thread, NULL))
    {
        /*Thread creation failed*/
        return 0;
    }

    return 1;
}


void finalize()
{
    EV_SET(&change, 1, EVFILT_TIMER, EV_DELETE, 0, timer.interval, 0);
    pthread_cancel(g_thread_id);
    pthread_join(g_thread_id, NULL);
}

void * _timer_thread(void * data)
{
    (void)(data);
    int kq, nev;
    kq = kqueue();
    EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, timer.interval, 0);
    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    nev = kevent(kq, &change, 1, &event, 1, NULL);

      if (nev < 0){
            diep("kevent()");
      }
      else if (nev > 0) {
        timer.callback((size_t)0, timer.user_data);
      }
    }
    return NULL;
}

void diep(const char *s)
{
   perror(s);
   exit(EXIT_FAILURE);
}
