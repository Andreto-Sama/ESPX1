#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define LOOP 20000

double timer;

void *producer (void *args);
void *consumer (void *args);

struct workFunction {
  void * (*work)(void *);
  void * arg;
};

typedef struct {
  struct workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

void *angler(void *arg){
  for (int i = 1; i < 11; i++)
    sin(i*10);
  //printf("done  \n");
}

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, struct workFunction in);
void queueDel (queue *q, struct workFunction *out);

int main (int argc, char *argv[])
{
  queue *fifo;
  int i;
  timer = 0;
  int p = atoi(argv[1]);
  int q = atoi(argv[2]);

  pthread_t *pro, *con;
  printf("%d producers, %d consumers\n", p,q);

  pro = (pthread_t *)malloc(p*sizeof(pthread_t));
  con = (pthread_t *)malloc(q*sizeof(pthread_t));

  //srand((unsigned int)time(NULL));

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  for (i = 0; i < p; i++) {
    pthread_create (&pro[i], NULL, producer, fifo);
  }
  for (i = 0; i < q; i++) {
    pthread_create (&con[i], NULL, consumer, fifo);
  }

  for (i = 0; i < p; i++) {
    pthread_join(pro[i],NULL);
  }

  for (i = 0; i < q; i++) {
    pthread_join(con[i],NULL);
  }

  queueDelete (fifo);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  //srand(time(0));
  struct timeval cur_time;

  struct workFunction w;
  w.work = angler;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      //printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    gettimeofday(&cur_time, NULL);
    timer -= (double)(cur_time.tv_usec/1.0e6 + cur_time.tv_sec);
    queueAdd (fifo, w);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }
  usleep (10000000);
  printf("%f \n", timer);
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  int i;
  struct workFunction d;
  struct timeval cur_time;

  fifo = (queue *)q;

  while(1){
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      //printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    gettimeofday(&cur_time, NULL);
    timer += (double)(cur_time.tv_usec/1.0e6 + cur_time.tv_sec);
    queueDel (fifo, &d);
    d.work(d.arg);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
  }
  return (NULL);
}


queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, struct workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, struct workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
