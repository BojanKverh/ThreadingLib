#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QDebug>

#include "sessionmanager.h"

#include "jobmanager.h"
#include "jobqueue.h"
#include "abstractjob.h"

//-----------------------------------------------------------------------------

class TestJob : public thr::AbstractJob
{
    Q_OBJECT

public:
    TestJob(unsigned int uiMax = 100) : thr::AbstractJob()
    {
        m_uiCount = 0;
        setMax(uiMax);
    }

    ~TestJob()
    {   }

    void setMax(unsigned int uiMax)
    {   m_uiMax = uiMax; }

    unsigned int max() const
    {   return m_uiMax; }

    int progress() const
    {   return 100*m_uiCount/(1 + m_uiMax); }

    unsigned int sum() const
    {   return m_uiSum; }

    void process()
    {
        m_bFinished = false;
        m_uiSum = 0;
        for (m_uiCount = 1; m_uiCount <= m_uiMax; ++m_uiCount) {
            m_uiSum += m_uiCount;
        }
    }

private:
    unsigned int m_uiCount;
    unsigned int m_uiMax;
    unsigned int m_uiSum;
};

//-----------------------------------------------------------------------------

class TestJobError : public thr::AbstractJob
{
    Q_OBJECT

public:
    TestJobError(int iMax = 100)
    {
        m_iCount = 0;
        setMax(iMax);
    }

    void setMax(int iMax)
    {   m_iMax = iMax; }

    int progress() const
    {   return 100*m_iCount/m_iMax; }

    void process()
    {
        int iSum = 0;
        for (m_iCount = 1; m_iCount <= m_iMax; ++m_iCount) {
            iSum += m_iCount;
        }

        if ((iSum % 2) > 0) {
            reportError(1);
        }
    }

private:
    int m_iCount;
    int m_iMax;

};

//-----------------------------------------------------------------------------

class TestJobCannotStart : public thr::AbstractJob
{
public:
    TestJobCannotStart() : thr::AbstractJob()
    {   }

    int progress() const
    {   return 0; }

    void process()
    {   }

    bool canStart() const
    {   return false; }
};

//-----------------------------------------------------------------------------

class TestJobSpawned : public thr::AbstractJob
{
public:
    TestJobSpawned() : thr::AbstractJob()
    {    }

    void process()
    {
        m_iSpawned = 0;
    }

    thr::AbstractJob* nextSpawnedJob()
    {
        ++m_iSpawned;
        if (m_iSpawned <= 2) {
            TestJob* pJob = new TestJob(1000*m_iSpawned);
            return pJob;
        }

        return 0;
    }

private:
    int m_iSpawned;
};

//-----------------------------------------------------------------------------

class UnitTestsTest : public QObject
{
    Q_OBJECT

public:
    UnitTestsTest();

    void clear();

public slots:
    void setFinished();
    void setError(thr::JobManagerError eJME);
    void setStop();
    void handleJobFinish(QSharedPointer<thr::AbstractJob> spJob);

private Q_SLOTS:
    void singleJobManagerStart();
    void singleJobManagerProcess();
    void jobManagerStart();
    void jobManagerProcess();
    void emptyJobManager();
    void jobQueueStart();
    void jobQueueProcess();
    void jobQueueStart2();
    void jobQueueStop();
    void errorsStart();
    void errorsStop();
    void jobManagerStart2();
    void jobManagerStop();
    void cannotStartJobStart();
    void cannotStartJobProcess();
    void fewJobsProcess();
    void dependencyProcess();
    void addThreads();
    void spawnJobs();
    void sessionTest();
    void sessionAddThreads();

private:
    void wait();

private:
    bool m_bFinished;
    thr::JobManagerError m_eError;
    bool m_bStop;

    thr::JobManager m_jm;

    thr::JobQueue* m_pQueue;

    QMutex m_mutex;
    QVector<int> m_vFinished;
};

//-----------------------------------------------------------------------------

UnitTestsTest::UnitTestsTest() : m_jm(0, this)
{
    connect(&m_jm, SIGNAL(signalFinished()), this, SLOT(setFinished()));
    connect(&m_jm, SIGNAL(signalError(thr::JobManagerError)), this, SLOT(setError(thr::JobManagerError)));
    connect(&m_jm, SIGNAL(signalStopped()), this, SLOT(setStop()));
}

//-----------------------------------------------------------------------------

void UnitTestsTest::clear()
{
    m_bFinished = false;
    m_eError = thr::jmeNoError;
    m_bStop = false;
    m_jm.clear();
    m_vFinished.clear();
}

//-----------------------------------------------------------------------------

void UnitTestsTest::setFinished()
{
    m_bFinished = true;
}

//-----------------------------------------------------------------------------

void UnitTestsTest::setError(thr::JobManagerError eJME)
{
    m_eError = eJME;
}

//-----------------------------------------------------------------------------

void UnitTestsTest::setStop()
{
    m_bStop = true;
}

//-----------------------------------------------------------------------------

void UnitTestsTest::handleJobFinish(QSharedPointer<thr::AbstractJob> spJob)
{
    QMutexLocker locker(&m_mutex);
    TestJob* pTJ = dynamic_cast<TestJob*>(spJob.data());

    m_vFinished << 7 - (pTJ->max()/100);
}

//-----------------------------------------------------------------------------

void UnitTestsTest::singleJobManagerStart()
{
    clear();
    TestJob* pJob = new TestJob;
    //QVERIFY2(m_sjm.startJob(&m_job), "Test job could not start!");
    m_jm.appendJob(pJob);
    QVERIFY2(m_jm.start(), "Test job could not start!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::singleJobManagerProcess()
{
    QTime tm;
    tm.start();
    // job should be finished in 1000 ms
    while ((tm.elapsed() < 1000) && (m_jm.isIdle() == false)) {
        wait();
    }
    QVERIFY2(m_bFinished == true, "Test job not finished yet!");
    QVERIFY2(m_eError == thr::jmeNoError, "Error signaled!");
    QVERIFY2(m_bStop == false, "Stop signaled!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobManagerStart()
{
    clear();
    for (int i = 0; i < 1000; ++i) {
        TestJob* pJob = new TestJob(i + 100);
        m_jm.appendJob(pJob);
    }

    QVERIFY2(m_jm.jobCount() == 1000, "Jobs not appended correctly!");
    QVERIFY2(m_jm.start() == true, "Job manager not started correctly!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobManagerProcess()
{
    QTime tm;
    tm.start();
    // job should be finished in 60 s
    while ((tm.elapsed() < 60000) && (m_jm.isIdle() == false)) {
        wait();
    }
    QVERIFY2(m_bFinished == true, "Job manager not finished correctly!");
    QVERIFY2(m_eError == thr::jmeNoError, "Error signaled!");
    QVERIFY2(m_bStop == false, "Stop signaled!");

    for (int i = 0; i < 1000; ++i) {
        QString qsJob = "Job %1 not finished!";
        QVERIFY2(m_jm.job(i)->isFinished(), qsJob.arg(i).toLatin1().constData());
    }
}

//-----------------------------------------------------------------------------

void UnitTestsTest::emptyJobManager()
{
    thr::JobManager jm;
    jm.start();
    QVERIFY2(jm.jobCount() == 0, "Empty job manager, job count not 0!");
    QVERIFY2(jm.finishedCount() == 0, "Empty job manager, finished jobs count not 0!");
    QVERIFY2(jm.isRunning() == false, "Empty job manager running when it should not!");
    QVERIFY2(jm.isFinished() == true, "Empty job manager not finished when it should be!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobQueueStart()
{
    clear();
    thr::JobQueue* pQueue = new thr::JobQueue;
    for (int i = 0; i < 50; ++i) {
        TestJob* pJob = new TestJob;
        pQueue->append(pJob);
    }
    m_jm.appendJob(pQueue);
    QVERIFY2(m_jm.start(), "Job queue not started!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobQueueProcess()
{
    QTime tm;
    tm.start();
    // job should be finished in 3000 ms
    while ((tm.elapsed() < 3000) && (m_jm.isIdle() == false)) {
        wait();
    }
    QVERIFY2(m_bFinished == true, "Job queue not finished!");
    QVERIFY2(m_eError == thr::jmeNoError, "Error signaled!");
    QVERIFY2(m_bStop == false, "Stop signaled!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobQueueStart2()
{
    clear();
    m_pQueue = new thr::JobQueue;
    for (int i = 0; i < 1900; ++i) {
        TestJob* pJob = new TestJob(2000 + i);
        m_pQueue->append(pJob);
    }
    m_jm.appendJob(m_pQueue);
    QVERIFY2(m_jm.start(), "Job queue not started!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobQueueStop()
{
    QTime tm;
    tm.start();
    // we only let the jobs run for 1 ms
    while ((tm.elapsed() < 1)) {
        QCoreApplication::instance()->processEvents();
    }
    m_jm.stop();
    while (tm.elapsed() < 100) {
        wait();
    }

    QVERIFY2(m_pQueue->isStopped() == true, "Job queue not stopped correctly!");
    QVERIFY2(m_bFinished == false, "Finish signaled!");
    QVERIFY2(m_eError == thr::jmeNoError, "Error signaled!");
    QVERIFY2(m_bStop == true, "Stop not signaled!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::errorsStart()
{
    clear();

    for (int i = 0; i < 1000; ++i) {
        m_jm.appendJob(new TestJobError(100 + i));
    }

    m_jm.setAllowedErrors(10);
    m_jm.start();
    QVERIFY2(m_jm.isIdle() == false, "JobManager not started!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::errorsStop()
{
    QTime tm;
    tm.start();
    // job should be finished in 5000 ms
    while ((tm.elapsed() < 5000) && (m_jm.isIdle() == false)) {
        wait();
    }

    QVERIFY2(m_bFinished == false, "Finished when it shouldn't!");
    QVERIFY2(m_eError == thr::jmeTooManyErrors, "Too many errors not signaled!");
    QVERIFY2(m_bStop == false, "Stop signaled!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobManagerStart2()
{
    clear();

    for (int i = 0; i < 1900; ++i) {
        m_jm.appendJob(new TestJob(i + 10));
    }

    m_jm.start();
    QVERIFY2(m_jm.isIdle() == false, "JobManager not started!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::jobManagerStop()
{
    QTime tm;
    tm.start();
    while ((tm.elapsed() < 1) && (m_jm.isIdle() == false)) {
        QCoreApplication::instance()->processEvents();
    }
    m_jm.stop();

    while (m_jm.isIdle() == false) {
        wait();
    }

    QVERIFY2(m_bFinished == false, "Finished when it shouldn't!");
    QVERIFY2(m_eError == thr::jmeNoError, "Error signaled when it shouldn't!");
    QVERIFY2(m_bStop == true, "Stop not signaled!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::cannotStartJobStart()
{
    clear();
    for (int i = 0; i < 10; ++i)
        m_jm.appendJob(new TestJob(i + 10));
    m_jm.appendJob(new TestJobCannotStart);
    for (int i = 10; i < 20; ++i)
        m_jm.appendJob(new TestJob(i + 20));
    m_jm.start();
    QVERIFY2(m_jm.isIdle() == false, "JobManager not started!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::cannotStartJobProcess()
{
    QTime tm;
    tm.start();
    while (m_jm.isIdle() == false) {
        wait();
    }

    QVERIFY2(m_bFinished == false, "Finished when it shouldn't!");
    QVERIFY2(m_eError == thr::jmeNoJobReady, "No job ready error not signaled!");
    QVERIFY2(m_bStop == false, "Stop signaled when it shouldn't!");
    QVERIFY2(m_jm.finishedCount() == 20, "Did not complete 20 jobs!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::fewJobsProcess()
{
    thr::JobManager jm(3);
    jm.appendJob(new TestJob(100));
    jm.appendJob(new TestJob(200));
    jm.appendJob(new TestJob(300));
    jm.start();
    while (jm.isRunning() == true) {
        wait();
    }
    QVERIFY2(dynamic_cast<TestJob*>(jm.job(0).data())->sum() == 5050, "sum(100) wrong");
    QVERIFY2(dynamic_cast<TestJob*>(jm.job(1).data())->sum() == 20100, "sum(200) wrong");
    QVERIFY2(dynamic_cast<TestJob*>(jm.job(2).data())->sum() == 45150, "sum(300) wrong");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::dependencyProcess()
{
    clear();
    thr::JobManager jm(5);
    connect(
                &jm,
                SIGNAL(signalJobFinished(QSharedPointer<thr::AbstractJob>)),
                this,
                SLOT(handleJobFinish(QSharedPointer<thr::AbstractJob>))
                );

    jm.setReportJobFinish(true);

    TestJob* pJob0 = new TestJob(700);
    TestJob* pJob1 = new TestJob(600);
    TestJob* pJob2 = new TestJob(500);
    TestJob* pJob3 = new TestJob(400);
    TestJob* pJob4 = new TestJob(300);
    TestJob* pJob5 = new TestJob(200);
    TestJob* pJob6 = new TestJob(100);
    jm.appendJob(pJob0);
    jm.appendJob(pJob1);
    jm.appendJob(pJob2);
    jm.appendJob(pJob3);
    jm.appendJob(pJob4);
    jm.appendJob(pJob5);
    jm.appendJob(pJob6);

    pJob4->addDependency(jm.job(0));
    pJob4->addDependency(jm.job(1));
    pJob6->addDependency(jm.job(2));
    pJob6->addDependency(jm.job(4));
    pJob5->addDependency(jm.job(6));
    pJob5->addDependency(jm.job(3));

    jm.start();
    while (jm.isRunning() == true) {
        wait();
    }

    QVERIFY2(m_vFinished.count() == 7, "Not all jobs reported finished!");
    QVERIFY2(m_vFinished.indexOf(4) < m_vFinished.indexOf(6), "Job 5 did not before job 7!");
    QVERIFY2(m_vFinished.indexOf(6) < m_vFinished.indexOf(5), "Job 7 did not finish before job 6!");
    QVERIFY2(m_vFinished.last() == 5, "Job 6 did not finish 7th!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::addThreads()
{
    clear();
    thr::JobManager jm(4);
    for (int i = 0; i < 100; ++i) {
        jm.appendJob(new TestJob(100 + 100*i));
    }
    jm.start();

    int iCnt = 0;
    int iMax = 0;
    while (jm.isRunning() == true) {
        wait();
        ++iCnt;
        if (iCnt == 5) {
            jm.addThreads(4);
        }
        if (iCnt > 5) {
            QVERIFY2(jm.threadCount() == 8, "Not enough threads!");
        }
        int iRunning = jm.threadsRunningCount();
        if (iRunning > iMax) {
            iMax = iRunning;
        }
    }

    QVERIFY2(
                dynamic_cast<TestJob*>(jm.job(99).data())->sum() == 5000*10001,
                "Sum not correct!"
                );
    QVERIFY2(iMax > 4, "Not enough threads used!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::spawnJobs()
{
    clear();
    thr::JobManager jm(4);

    jm.appendJob(new TestJobSpawned);

    jm.start();
    while (jm.isRunning() == true) {
        wait();
    }

    QVERIFY2(jm.jobCount() == 3, "Not enough jobs spawned!");
    QVERIFY2(jm.finishedCount() == 3, "Not enough jobs processed!");
    QVERIFY2(jm.job(0)->isSpawned() == false, "First job should not be spawned");
    QVERIFY2(jm.job(1)->isSpawned() == true, "Second job should be spawned");
    QVERIFY2(jm.job(2)->isSpawned() == true, "Third job should be spawned");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::sessionTest()
{
    SessionManager sm;
    sm.start();

    while (sm.isRunning() == true) {
        wait();
    }

    QVERIFY2(sm.isFinished() == true, "Session manager not finished correctly!");
    QVERIFY2(sm.currentSession() == 3, "Not all sessions finished!");
    QVERIFY2(sm.finishedJobs() == 350, "Number of finished jobs not right!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::sessionAddThreads()
{
    QTime tm;

    SessionManager sm(4);
    sm.start();
    tm.start();

    int iMax1 = 0;
    int iMax2 = 0;
    int iCnt = 0;

    while (sm.isRunning() == true) {
        wait();
        ++iCnt;
        int iT = sm.threadsRunningCount();
        if (iCnt <= 100) {
            if (iT > iMax1)
                iMax1 = iT;

            if (iCnt == 100)
                sm.addThreads(4);
        }   else {
            if (iT > iMax2)
                iMax2 = iT;
        }
    }

    QVERIFY2(sm.isFinished() == true, "Session manager not finished correctly!");
    QVERIFY2(sm.currentSession() == 3, "Not all sessions finished!");
    QVERIFY2(sm.finishedJobs() == 350, "Number of finished jobs not right!");
    QVERIFY2(iMax1 <= 4, "Number of threads used in first stage too big!");
    QVERIFY2(iMax2 > 4, "Number of threads used in second stage too low!");
}

//-----------------------------------------------------------------------------

void UnitTestsTest::wait()
{
    QCoreApplication::instance()->processEvents();
}

//-----------------------------------------------------------------------------

QTEST_MAIN(UnitTestsTest)

#include "unittests.moc"
