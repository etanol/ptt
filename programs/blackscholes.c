/*
 * Copyright (c) 2007 Intel Corp.
 *
 * Black-Scholes
 * Analytical method for calculating European Options
 *
 * Reference Source: Options, Futures, and Other Derivatives, 3rd Edition,
 * Prentice Hall, John C. Hull,
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS  128
#define NUM_RUNS     100
#define fptype       float

static pthread_t threadsTable[MAX_THREADS];
static pthread_mutexattr_t normalMutexAttr;
static int numThreads = MAX_THREADS;
static pthread_barrier_t barrier;


typedef struct OptionData_
{
        fptype s;         /* spot price */
        fptype strike;    /* strike price */
        fptype r;         /* risk-free interest rate */
        fptype divq;      /* dividend rate */
        fptype v;         /* volatility */
        fptype t;         /* time to maturity or option expiration in years
                           * (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, etc) */
        int OptionType;   /* Option type.  "P"=PUT, "C"=CALL */
        fptype divs;      /* dividend vals (not used in this test) */
        fptype DGrefval;  /* DerivaGem Reference Value */
} OptionData;

static OptionData data_init[] = {
#include "optionData.txt"
};

static OptionData *data;
static fptype *prices;
static int numOptions;
static int *otype;
static fptype *sptprice;
static fptype *strike;
static fptype *rate;
static fptype *volatility;
static fptype *otime;
static int nThreads;
#ifdef DEBUG
static int numError = 0;
#endif



/*
 * Cumulative Normal Distribution Function
 * See Hull, Section 11.8, P.243-244
 */
#define inv_sqrt_2xPI 0.39894228040143270286

fptype CNDF (fptype InputX)
{
        int sign;
        fptype OutputX;
        fptype xInput;
        fptype xNPrimeofX;
        fptype expValues;
        fptype xK2;
        fptype xK2_2, xK2_3;
        fptype xK2_4, xK2_5;
        fptype xLocal, xLocal_1;
        fptype xLocal_2, xLocal_3;

        /* Check for negative value of InputX */
        if (InputX < 0.0)
        {
                InputX = -InputX;
                sign = 1;
        }
        else
                sign = 0;

        xInput = InputX;

        /* Compute NPrimeX term common to both four & six decimal accuracy calcs */
        expValues = exp(-0.5f * InputX * InputX);
        xNPrimeofX = expValues;
        xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

        xK2 = 0.2316419 * xInput;
        xK2 = 1.0 + xK2;
        xK2 = 1.0 / xK2;
        xK2_2 = xK2 * xK2;
        xK2_3 = xK2_2 * xK2;
        xK2_4 = xK2_3 * xK2;
        xK2_5 = xK2_4 * xK2;

        xLocal_1 = xK2 * 0.319381530;
        xLocal_2 = xK2_2 * (-0.356563782);
        xLocal_3 = xK2_3 * 1.781477937;
        xLocal_2 = xLocal_2 + xLocal_3;
        xLocal_3 = xK2_4 * (-1.821255978);
        xLocal_2 = xLocal_2 + xLocal_3;
        xLocal_3 = xK2_5 * 1.330274429;
        xLocal_2 = xLocal_2 + xLocal_3;

        xLocal_1 = xLocal_2 + xLocal_1;
        xLocal = xLocal_1 * xNPrimeofX;
        xLocal = 1.0 - xLocal;

        OutputX = xLocal;

        if (sign)
                OutputX = 1.0 - OutputX;

        return OutputX;
}


/*
 * For debugging.
 */
void print_xmm (fptype in, char *s)
{
        printf("%s: %f\n", s, in);
}


/*
 * Real computation.
 */
fptype BlkSchlsEqEuroNoDiv (fptype sptprice, fptype strike, fptype rate,
                            fptype volatility, fptype time, int otype,
                            float timet)
{
        fptype OptionPrice;
        /* local private working variables for the calculation */
        fptype xStockPrice;
        fptype xStrikePrice;
        fptype xRiskFreeRate;
        fptype xVolatility;
        fptype xTime;
        fptype xSqrtTime;
        fptype logValues;
        fptype xLogTerm;
        fptype xD1;
        fptype xD2;
        fptype xPowerTerm;
        fptype xDen;
        fptype d1;
        fptype d2;
        fptype FutureValueX;
        fptype NofXd1;
        fptype NofXd2;
        fptype NegNofXd1;
        fptype NegNofXd2;

        xStockPrice = sptprice;
        xStrikePrice = strike;
        xRiskFreeRate = rate;
        xVolatility = volatility;

        xTime = time;
        xSqrtTime = sqrt(xTime);

        logValues = log(sptprice / strike);

        xLogTerm = logValues;

        xPowerTerm = xVolatility * xVolatility;
        xPowerTerm = xPowerTerm * 0.5;

        xD1 = xRiskFreeRate + xPowerTerm;
        xD1 = xD1 * xTime;
        xD1 = xD1 + xLogTerm;

        xDen = xVolatility * xSqrtTime;
        xD1 = xD1 / xDen;
        xD2 = xD1 - xDen;

        d1 = xD1;
        d2 = xD2;

        NofXd1 = CNDF(d1);
        NofXd2 = CNDF(d2);

        FutureValueX = strike * (exp(-(rate) * (time)));
        if (otype == 0)
                OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
        else
        {
                NegNofXd1 = (1.0 - NofXd1);
                NegNofXd2 = (1.0 - NofXd2);
                OptionPrice = (FutureValueX * NegNofXd2) -
                              (sptprice * NegNofXd1);
        }

        return OptionPrice;
}


/*
 * The thread function.
 */
void *bs_thread (void *tid_ptr)
{
        int i, j;
        fptype price;
#ifdef DEBUG
        fptype priceDelta;
#endif
        int tid = *(int *) tid_ptr;
        int start = tid * (numOptions / nThreads);
        int end = start + (numOptions / nThreads);

        pthread_barrier_wait(&barrier);

        for (j = 0;  j < NUM_RUNS;  j++)
                for (i = start;  i < end;  i++)
                {
                        /* Calling main function to calculate option value based on
                         * Black & Sholes's equation */
                        price = BlkSchlsEqEuroNoDiv(sptprice[i], strike[i],
                                                    rate[i], volatility[i],
                                                    otime[i], otype[i], 0);
                        prices[i] = price;
#ifdef DEBUG
                        priceDelta = data[i].DGrefval - price;
                        if (fabs(priceDelta) >= 1e-4)
                        {
                                printf("Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n",
                                       i, price, data[i].DGrefval, priceDelta);
                                numError++;
                        }
#endif
                }

        return NULL;
}


/*
 * Main function
 */
int main (int argc, char **argv)
{
        int i, tid, *tids;
        int loopnum;
        fptype *buffer;
        int *buffer2;
        int initOptionNum;
        void *ret;

        if (argc != 3)
        {
                printf("Usage:\n\t%s <nthreads> <numOptions>\n", argv[0]);
                exit(1);
        }
        nThreads = atoi(argv[1]);
        numOptions = atoi(argv[2]);

        if (nThreads > numOptions)
        {
                printf("WARNING: Not enough work, reducing number of threads to match number of options.");
                nThreads = numOptions;
        }

        /* alloc spaces for the option data */
        tids = malloc(nThreads * sizeof(int));
        data = malloc(numOptions * sizeof(OptionData));
        prices = malloc(numOptions * sizeof(fptype));
        /* initialize the data array */
        initOptionNum = ((sizeof(data_init)) / sizeof(OptionData));
        for (loopnum = 0;  loopnum < numOptions;  loopnum++)
        {
                OptionData *temp = data_init + loopnum % initOptionNum;

                data[loopnum].OptionType = temp->OptionType;
                data[loopnum].s = temp->s;
                data[loopnum].strike = temp->strike;
                data[loopnum].r = temp->r;
                data[loopnum].divq = temp->divq;
                data[loopnum].v = temp->v;
                data[loopnum].t = temp->t;
                data[loopnum].divs = temp->divs;
                data[loopnum].DGrefval = temp->DGrefval;
        }

        pthread_mutexattr_init(&normalMutexAttr);
        numThreads = nThreads;
        for (i = 0;  i < MAX_THREADS;  i++)
                threadsTable[i] = -1;

        pthread_barrier_init(&(barrier), NULL, numThreads);;
        printf("Num of Options: %d\n", numOptions);
        printf("Num of Runs: %d\n", NUM_RUNS);

#define PAD 256
#define LINESIZE 64

        buffer = (fptype *) malloc(5 * numOptions * sizeof(fptype) + PAD);
        sptprice = (fptype *) (((unsigned long) buffer + PAD) & ~(LINESIZE - 1));
        strike = sptprice + numOptions;
        rate = strike + numOptions;
        volatility = rate + numOptions;
        otime = volatility + numOptions;

        buffer2 = (int *) malloc(numOptions * sizeof(fptype) + PAD);
        otype = (int *) (((unsigned long) buffer2 + PAD) & ~(LINESIZE - 1));

        for (i = 0;  i < numOptions;  i++)
        {
                otype[i] = (data[i].OptionType == 'P') ? 1 : 0;
                sptprice[i] = data[i].s;
                strike[i] = data[i].strike;
                rate[i] = data[i].r;
                volatility[i] = data[i].v;
                otime[i] = data[i].t;
        }

        printf("Size of data: %d\n",
               numOptions * (sizeof(OptionData) + sizeof(int)));


        for (tid = 0;  tid < nThreads;  tid++)
        {
                tids[tid] = tid;
                for (i = 0;  i < MAX_THREADS;  i++)
                        if (threadsTable[i] == -1)
                                break;
                pthread_create(&threadsTable[i], NULL, bs_thread, &tids[i]);

        }

        for (i = 0;  i < MAX_THREADS;  i++)
        {
                if (threadsTable[i] == -1)
                        break;
                pthread_join(threadsTable[i], &ret);
        }

        tid = 0;
        bs_thread(&tid);

#ifdef DEBUG
        printf("Num Errors: %d\n", numError);
#endif
        free(data);
        free(prices);
        free(tids);

        return 0;
}

