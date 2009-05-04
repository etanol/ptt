/*
 * matmul.c
 *
 * Unoptimized matrix matrix multiplication.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

#define MAX_THREAD  20
#define NDIM        500

double a[NDIM][NDIM];
double b[NDIM][NDIM];
double c[NDIM][NDIM];

typedef struct
{
        int id;
        int noproc;
        int dim;
        double (*a)[NDIM][NDIM];
        double (*b)[NDIM][NDIM];
        double (*c)[NDIM][NDIM];
} parm;

void check_matrix (int);
void print_matrix (int);


void mm (int me_no,int noproc, int n, double a[NDIM][NDIM], double b[NDIM][NDIM],
         double c[NDIM][NDIM])
{
        int i, j, k;
        double sum;

        ptt_event(PHASE, MAIN_LOOP);
        i = me_no;
        while (i < n)
        {
                for (j = 0;  j < n;  j++)
                {
                        ptt_event(ITERATION, j);
                        sum = 0.0;
                        for (k = 0; k < n; k++)
                                sum = sum + a[i][k] * b[k][j];
                        c[i][j] = sum;

                }
                i += noproc;
        }
        ptt_event(PHASE, END);
}


void * worker (void *arg)
{
        parm *p = (parm *) arg;
        mm(p->id, p->noproc, p->dim, *(p->a), *(p->b), *(p->c));
        return NULL;
}


int main (int argc, char **argv)
{
        int j, n, i;
        pthread_t *threads;
        pthread_attr_t pthread_custom_attr;
        parm *arg;

        ptt_event(PHASE, SETUP);
        for (i = 0;  i < NDIM;  i++)
                for (j = 0;  j < NDIM;  j++)
                {
                        a[i][j] = i + j;
                        b[i][j] = i + j;
                }

        if (argc != 2)
        {
                printf("Usage: %s n\n  where n is no. of thread\n", argv[0]);
                exit(1);
        }
        n = atoi(argv[1]);

        if (n < 1 || n > MAX_THREAD)
        {
                printf("The no of thread should between 1 and %d.\n",
                       MAX_THREAD);
                exit(1);
        }
        threads = (pthread_t *) malloc(n * sizeof(pthread_t));
        pthread_attr_init(&pthread_custom_attr);

        arg = (parm *) malloc(sizeof(parm) * n);
        /* setup barrier */

        /* Start up thread */

        /* Spawn thread */
        ptt_event(PHASE, THREAD_CREATION);
        for (i = 0;  i < n;  i++)
        {
                arg[i].id = i;
                arg[i].noproc = n;
                arg[i].dim = NDIM;
                arg[i].a = &a;
                arg[i].b = &b;
                arg[i].c = &c;
                pthread_create(&threads[i], &pthread_custom_attr, worker,
                               (void *) (arg + i));
        }

        ptt_event(PHASE, THREAD_WAIT);
        for (i = 0;  i < n;  i++)
                pthread_join(threads[i], NULL);
        /* print_matrix(NDIM); */
        ptt_event(PHASE, CHECK);
        check_matrix(NDIM);
        free(arg);
        return 0;
}


void print_matrix (int dim)
{
        int i, j;

        printf("The %d * %d matrix is\n", dim, dim);
        for (i = 0;  i < dim;  i++)
        {
                for (j = 0;  j < dim;  j++)
                        printf("%lf ", c[i][j]);
                printf("\n");
        }
}


void check_matrix (int dim)
{
        int i, j, k;
        double e;
        int error = 0;

        printf("Now checking the results\n");
        for (i = 0;  i < dim;  i++)
                for (j = 0;  j < dim;  j++)
                {
                        e = 0.0;
                        for (k = 0;  k < dim;  k++)
                                e += a[i][k] * b[k][j];
                        if (e != c[i][j])
                        {
                                printf("(%d,%d) error\n", i, j);
                                error++;
                        }
                }

        if (error)
                printf("%d elements error\n", error);
        else
                printf("success\n");
}

