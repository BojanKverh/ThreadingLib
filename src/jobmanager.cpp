#include <assert.h>

#include <QVariant>
#include <QDebug>

#include "jobmanager.h"

#define THREAD_INDEX            "thInd"

namespace thr {

//-----------------------------------------------------------------------------

JobManager::JobManager(int iThreads, QObject* pParent) : QObject(pParent)
{
    m_timer.setInterval(0);
    m_iStarted = 0;
    m_iRunning = 0;
    m_bStop = false;
    m_bReportJobFinish = false;
    m_eStatus = sFinished;
    m_eError = jmeNoError;
    m_iFinished = 0;
    m_iAllowedErrors = 0;
    if (iThreads <= 0) {
        iThreads = QThread::idealThreadCount();
    }
    allocateThreads(iThreads);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(reportProgress()));
}

//-----------------------------------------------------------------------------

JobManager::~JobManager()
{
    clear();
    QMutexLocker locker(&m_mutex);
    m_vThreads.clear();
}

//-----------------------------------------------------------------------------

void JobManager::appendJob(AbstractJob* pJob)
{
    QMutexLocker locker(&m_mutex);
    QSharedPointer<AbstractJob> spJob(pJob);
    appendJobUnsafe(spJob);
}

//-----------------------------------------------------------------------------

void JobManager::clear()
{
    QMutexLocker locker(&m_mutex);
    m_vspJobs.clear();
    m_quWaiting.clear();
    m_iStarted = 0;
    m_iRunning = 0;
    m_bStop = false;
    m_eError = jmeNoError;
}

//-----------------------------------------------------------------------------

void JobManager::setAllowedErrors(int iN)
{
    m_iAllowedErrors = iN;
}

//-----------------------------------------------------------------------------

void JobManager::setProgressReportTimeout(int iMS)
{
    m_timer.setInterval(iMS);
}

//-----------------------------------------------------------------------------

void JobManager::setThreads(int iT)
{
    if (m_eStatus == sRunning) {
        // cannot change the number of threads while processing!
        return;
    }
    allocateThreads(iT);
}

//-----------------------------------------------------------------------------

void JobManager::addThreads(int iT)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < iT; ++i) {
        auto spThr = QSharedPointer<Thread>(new Thread);
        m_vThreads.append(spThr);
        m_quIdle.enqueue(spThr);
        if (isRunning() == true) {
            startNext();
        }
    }
}

//-----------------------------------------------------------------------------

int JobManager::threadsRunningCount() const
{
    QMutexLocker locker(&m_mutex);
    int iCnt = 0;
    for (int i = 0; i < m_vThreads.count(); ++i) {
        if (m_vThreads[i]->isRunning() == true) {
            ++iCnt;
        }
    }

    return iCnt;
}

//-----------------------------------------------------------------------------

bool JobManager::start()
{
    if (m_eStatus == sRunning) {
        // cannot start while already processing!
        return false;
    }
    m_eStatus = sRunning;
    m_iErrors = 0;
    m_iStarted = 0;
    m_iFinished = 0;
    m_iRunning = 0;
    m_bStop = false;
    m_eError = jmeNoError;

    if (m_vspJobs.count() == 0) {
        // nothing to do
        m_eStatus = sFinished;
        emit signalFinished();
        return true;
    }

    int iN = qMin(m_vThreads.count(), m_vspJobs.count());
    for (int i = 0; i < iN; ++i) {
        startNext();
    }

    if (m_timer.interval() > 0) {
        m_timer.start();
    }
    return true;
}

//-----------------------------------------------------------------------------

void JobManager::stop()
{
    QMutexLocker locker(&m_mutex);
    //m_eStatus = sStopped;
    m_bStop = true;

    for (int i = 0; i < m_vThreads.count(); ++i) {
        //m_vThreads[i]->disconnect();
        //m_vThreads[i]->quit();
        //m_vThreads[i]->wait();
        //m_vJobs[m_vIndex[i]]->stop();
        if (m_vThreads[i]->jobIndex() >= 0) {
            m_vspJobs[m_vThreads[i]->jobIndex()]->stop();
        }
    }
    //emit signalFinished();
}

//-----------------------------------------------------------------------------

void JobManager::handleJobFinished()
{
    QMutexLocker locker(&m_mutex);

    ++m_iFinished;
    Thread* pThr = dynamic_cast<Thread*>(sender());
    QSharedPointer<Thread> spThr;
    for (int i = 0; i < m_vThreads.count(); ++i) {
        if (m_vThreads[i].data() == pThr) {
            spThr = m_vThreads[i];
        }
    }
    assert(spThr.data() != 0);
    int iInd = spThr->jobIndex();

    int iCnt = 0;
    AbstractJob* pJob = m_vspJobs[iInd]->nextSpawnedJob();
    while (pJob != nullptr) {
        ++iCnt;
        pJob->setSpawned();
        QSharedPointer<AbstractJob> spJob(pJob);
        appendJobUnsafe(spJob);
        pJob = m_vspJobs[iInd]->nextSpawnedJob();
    }

    m_vspJobs[iInd]->cleanup();

    m_quIdle.enqueue(spThr);
    --m_iRunning;

    if (m_vspJobs[iInd]->isError() == true) {
        ++m_iErrors;
    }

    if (m_bReportJobFinish == true) {
        emit signalJobFinished(m_vspJobs[iInd]);
    }

    int iN = qMax(1, qMin(m_quWaiting.count(), m_quIdle.count()));
    for (int i = 0; i < iN; ++i)
        checkNext();

    if (m_eStatus == sFinished) {
        locker.unlock();
        emit signalFinished();
    }
}

//-----------------------------------------------------------------------------

void JobManager::reportProgress()
{
    if (m_vspJobs.count() > 0) {
        emit signalProgress(100*m_iFinished/m_vspJobs.count());
    }
}

//-----------------------------------------------------------------------------

void JobManager::checkNext()
{
    if ((m_iAllowedErrors >= 0) && (m_iErrors > m_iAllowedErrors)) {
        m_eError = jmeTooManyErrors;
    }

    if (handleError() == true) {
        // prevent new jobs being started if an error occured
        return;
    }

    if (m_bStop == true) {
        if (m_iRunning == 0) {
            m_eStatus = sStopped;
            emit signalStopped();
        }
        // prevent new jobs being started if processing was stopped by the caller
        return;
    }

    if (m_iFinished < m_vspJobs.count()) {
        startNext();
        if (handleError() == true) {
            // prevent new jobs being started if an error occured
            return;
        }
    }   else {
        if (m_timer.interval() > 0) {
            emit signalProgress(100);
            m_timer.stop();
        }
        m_eStatus = sFinished;
        //emit signalFinished();
    }
}

//-----------------------------------------------------------------------------

void JobManager::startNext()
{
    auto spThr = m_quIdle.dequeue();
    spThr->disconnect();
    if (m_iStarted < m_vspJobs.count()) {
        for (int i = 0; i < m_quWaiting.count(); ++i) {
            if (m_vspJobs[m_quWaiting.front()]->canStart() == true) {
                int iCurrent = m_quWaiting.dequeue();
                //m_vJobs[iCurrent]->moveToThread(pThr);
                //connect(pThr, SIGNAL(started()), m_vJobs[iCurrent], SLOT(start()));
                //connect(m_vJobs[iCurrent], SIGNAL(signalFinished()), pThr, SLOT(quit()));
                //connect(m_vJobs[iCurrent], SIGNAL(signalStopped()), pThr, SLOT(quit()));
                //connect(m_vJobs[iCurrent], SIGNAL(signalError()), pThr, SLOT(quit()));
                //connect(pThr, SIGNAL(finished()), this, SLOT(handleJobFinished()));
                connect(spThr.data(), &QThread::finished, this, &JobManager::handleJobFinished);
                //m_vIndex[iInd] = iCurrent;
                //pThr->start();
                spThr->start(iCurrent, m_vspJobs[iCurrent]);
                ++m_iStarted;
                ++m_iRunning;
                return;
            }   else {
                m_quWaiting.enqueue(m_quWaiting.dequeue());
            }
        }
        // should not get here!
        if (m_iRunning == 0) {
            qWarning() << "JobManager: could not find job to start, unfinished jobs left: " << m_quWaiting.count();
            m_eError = jmeNoJobReady;
        }
    }
    m_quIdle.enqueue(spThr);
}

//-----------------------------------------------------------------------------

void JobManager::allocateThreads(int iT)
{
    m_vThreads.clear();
    m_quIdle.clear();
    //m_vIndex.clear();

    for (int i = 0; i < iT; ++i) {
        auto spThr = QSharedPointer<Thread>(new Thread);
        m_vThreads.append(spThr);
        m_quIdle.enqueue(spThr);
       // m_vIndex.append(-1);
    }
}

//-----------------------------------------------------------------------------

bool JobManager::handleError()
{
    if (m_eError != jmeNoError) {
        if (m_iRunning == 0) {
            m_eStatus = sError;
            emit signalError(m_eError);
        }
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

void JobManager::appendJobUnsafe(QSharedPointer<AbstractJob> spJob)
{
    m_quWaiting.enqueue(m_vspJobs.count());
    m_vspJobs.append(spJob);
}

//-----------------------------------------------------------------------------

}   // namespace

