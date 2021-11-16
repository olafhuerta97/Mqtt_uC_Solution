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


int initialize()
{
    if(pthread_create(&g_thread_id, NULL, _timer_thread, NULL))
    {
        /*Thread creation failed*/
        return 0;
    }

    return 1;
}

size_t start_timer(unsigned int interval, time_handler handler, void * user_data)
{
    
   struct kevent change;    /* event we want to monitor */
   struct kevent event;     /* event that was triggered */



    timer.callback  = handler;
    timer.user_data = user_data;
    timer.interval  = interval;
   // EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 5000, 0);

    timer.fd = change;


    return (size_t)1;
}



void finalize()
{
    pthread_cancel(g_thread_id);
    pthread_join(g_thread_id, NULL);
}


void * _timer_thread(void * data)
{
    struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
    int read_fds = 0, i, s;
    uint64_t exp;
    int kq, nev;
    pid_t pid;
    kq = kqueue();
            EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 5000, 0);
    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        nev = kevent(kq, &change, 1, &event, 1, NULL);
		printf("while kevent\n");

      if (nev < 0)
         diep("kevent()");

      else if (nev > 0) {
         if (event.flags & EV_ERROR) {   /* report any error */
            fprintf(stderr, "EV_ERROR: %s\n", strerror(event.data));
            exit(EXIT_FAILURE);
         }
		 printf("while loop\n");

         if ((pid = fork()) < 0)         /* fork error */
            diep("fork()");

         else if (pid == 0)              /* child */
           timer.callback((size_t)0, timer.user_data);
      }
/*compiles but works weird */
    }

    return NULL;
}

void diep(const char *s)
{
   perror(s);
   exit(EXIT_FAILURE);
}
