#define _MULTI_THREADED
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMTHREADS  2

pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dataPresentCondition = PTHREAD_COND_INITIALIZER;
int dataPresent = 0;
int sharedData = 0;


static inline void checkResults (const char *string, int val)
{
        if (val)
        {
                printf("Failed with %d at %s", val, string);
                exit(1);
        }
}


void *theThread (void *parm)
{
        int rc;
        int retries = 2;

        ptt_event(STATE, RUNNING);
        printf("Consumer Thread %.8x: Entered\n",
               (unsigned int) pthread_self());
        ptt_event(STATE, LOCKING);
        rc = pthread_mutex_lock(&dataMutex);
        ptt_event(STATE, MUTUAL_EXCLUSION);
        checkResults("pthread_mutex_lock()\n", rc);

        while (retries--)
        {
                /*
                 * The boolean dataPresent value is required for safe use of
                 * condition variables.  If no data is present we wait, other
                 * wise we process immediately.
                 */
                while (!dataPresent)
                {
                        //printf("Consumer Thread %.8x %.8x: Wait for data to be produced\n");
                        ptt_event(STATE, COND_WAITING);
                        rc = pthread_cond_wait(&dataPresentCondition,
                                               &dataMutex);
                        ptt_event(STATE, COND_OK);
                        if (rc)
                        {
                                //printf("Consumer Thread %.8x %.8x: condwait failed, rc=%d\n", rc);
                                ptt_event(STATE, UNLOCKING);
                                pthread_mutex_unlock(&dataMutex);
                                ptt_event(STATE, RUNNING);
                                exit(1);
                        }
                }
                printf("Consumer Thread %.8x: Found data or Notified, "
                       "CONSUME IT while holding lock\n", (unsigned int) pthread_self());

                /*
                 * Typically an application should remove the data from being in
                 * the shared structure or Queue, then unlock.  Processing of
                 * the data does not necessarily require that the lock is held.
                 * Access to shared data goes here.
                 */
                --sharedData;
                /* We consumed the last of the data */
                if (sharedData == 0)
                        dataPresent = 0;
                /* Repeat holding the lock, pthread_cond_wait releases it
                 * atomically */
        }
        printf("Consumer Thread %.8x: All done\n", (unsigned int) pthread_self());
        ptt_event(STATE, UNLOCKING);
        rc = pthread_mutex_unlock(&dataMutex);
        ptt_event(STATE, CHECK);
        checkResults("pthread_mutex_unlock()\n", rc);
        ptt_event(STATE, RUNNING);
        return NULL;
}


int main (int argc, char **argv)
{
        pthread_t thread[NUMTHREADS];
        int rc = 0;
        int amountOfData = 4;
        int i;

        ptt_event(STATE, RUNNING);
        printf("Enter Testcase - %s\n", argv[0]);

        ptt_event(STATE, FORKING);
        printf("Create/start threads\n");
        for (i = 0;  i < NUMTHREADS;  i++)
        {
                rc = pthread_create(&thread[i], NULL, theThread, NULL);
                checkResults("pthread_create()\n", rc);
        }

        /* The producer loop */
        while (amountOfData--)
        {
                ptt_event(STATE, SLEEPING);
                printf("Producer: 'Finding' data\n");
                sleep(3);
                ptt_event(STATE, RUNNING);

                /* Protect shared data and flag */
                ptt_event(STATE, LOCKING);
                rc = pthread_mutex_lock(&dataMutex);
                ptt_event(STATE, MUTUAL_EXCLUSION);
                checkResults("pthread_mutex_lock()\n", rc);
                printf("Producer: Make data shared and notify consumer\n");
                /* Add data */
                sharedData++;
                /* Set boolean predicate */
                dataPresent = 1;

                /* Wake up a consumer */
                ptt_event(STATE, COND_WAITING);
                rc = pthread_cond_signal(&dataPresentCondition);
                ptt_event(STATE, COND_OK);
                if (rc)
                {
                        ptt_event(STATE, UNLOCKING);
                        pthread_mutex_unlock(&dataMutex);
                        ptt_event(STATE, RUNNING);
                        printf("Producer: Failed to wake up consumer, rc=%d\n",
                               rc);
                        exit(1);
                }

                printf("Producer: Unlock shared data and flag\n");
                ptt_event(STATE, UNLOCKING);
                rc = pthread_mutex_unlock(&dataMutex);
                ptt_event(STATE, CHECK);
                checkResults("pthread_mutex_lock()\n", rc);
        }

        printf("Wait for the threads to complete, and release their resources\n");
        for (i = 0;  i < NUMTHREADS;  i++)
        {
                ptt_event(STATE, THREAD_WAITING);
                rc = pthread_join(thread[i], NULL);
                ptt_event(STATE, CHECK);
                checkResults("pthread_join()\n", rc);
        }
        ptt_event(STATE, RUNNING);

        printf("Clean up\n");
        rc = pthread_mutex_destroy(&dataMutex);
        rc = pthread_cond_destroy(&dataPresentCondition);
        printf("Main completed\n");
        return 0;
}

