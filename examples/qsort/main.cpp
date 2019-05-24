#include <stdlib.h>
#include <assert.h>

#include <QCoreApplication>
#include <QTime>

#include "jobsort.h"
#include "jobmanager.h"

#define N   50000000

int compare(const void* pi1, const void* pi2)
{
    return *(int*)pi1 - *(int*)pi2;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    thr::JobManager jm(8);

    int* paiN1 = new int[N];
    int* paiN2 = new int[N];
    for (int i = 0; i < N; ++i) {
        paiN1[i] = paiN2[i] = rand() % (10*N);
    }

    QTime tm;
    tm.start();
    qsort(paiN1, N, sizeof(int), compare);
    qDebug() << "System qsort time elapsed" << tm.elapsed() << "[ms]";

    tm.start();

    JobSort* pJob = new JobSort(paiN2, 0, N - 1);
    jm.appendJob(pJob);
    jm.start();

    while (jm.isRunning() == true) {
        a.processEvents();
    }
    qDebug() << "Multithreaded qsort time elapsed" << tm.elapsed() << "[ms]";

    // check the correctness
    for (int i = 1; i < N; ++i) {
        if (paiN2[i - 1] > paiN2[i])
            qDebug() << "Error at" << i;
        assert(paiN2[i - 1] <= paiN2[i]);
    }

    return 0;
}
