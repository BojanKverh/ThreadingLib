#include <QDebug>

#include "jobqueue.h"

namespace thr {

//-----------------------------------------------------------------------------

JobQueue::JobQueue() : AbstractJob()
{
    m_iCurrent = -1;
}

//-----------------------------------------------------------------------------

JobQueue::~JobQueue()
{
    m_vJobs.clear();
}

//-----------------------------------------------------------------------------

void JobQueue::append(AbstractJob* pJob)
{
    QMutexLocker locker(&m_mutex);
    QSharedPointer<AbstractJob> spJob(pJob);
    m_vJobs.append(spJob);
}

//-----------------------------------------------------------------------------

void JobQueue::clear()
{
    QMutexLocker locker(&m_mutex);
    m_iCurrent = -1;
    m_vJobs.clear();
}

//-----------------------------------------------------------------------------

int JobQueue::progress() const
{
    if (m_iCurrent < 0)
        return 0;
    else if (m_iCurrent >= m_vJobs.count())
        return 100;

    return (100*m_iCurrent + m_vJobs[m_iCurrent]->progress())/m_vJobs.count();
}

//-----------------------------------------------------------------------------

void JobQueue::process()
{
    QMutexLocker locker(&m_mutex);
    for (m_iCurrent = 0; (m_iCurrent < m_vJobs.count()) && (m_iError == 0); ++m_iCurrent) {
        CHECK_JOB_STOP();
        m_vJobs[m_iCurrent]->process();
        if (m_vJobs[m_iCurrent]->errorCode() > 0) {
            m_iError = m_vJobs[m_iCurrent]->errorCode();
        }
    }
}

//-----------------------------------------------------------------------------

}   // namespace

