#include <QDebug>

#include "thread.h"

namespace thr {

//-----------------------------------------------------------------------------

Thread::Thread() : QThread()
{
    m_iJobIndex = -1;
    m_spJob = 0;
}

//-----------------------------------------------------------------------------

Thread::~Thread()
{   }

//-----------------------------------------------------------------------------

void Thread::start(int iJobIndex, QSharedPointer<AbstractJob> spJob)
{
    m_iJobIndex = iJobIndex;
    m_spJob = spJob;
    if (m_spJob != 0) {
        m_spJob->moveToThread(this);
        connect(this, SIGNAL(started()), m_spJob.data(), SLOT(exec()));
        connect(m_spJob.data(), SIGNAL(signalFinished()), this, SLOT(quit()));
        connect(m_spJob.data(), SIGNAL(signalStopped()), this, SLOT(quit()));
        connect(m_spJob.data(), SIGNAL(signalError()), this, SLOT(quit()));
        QThread::start();
    }
}

//-----------------------------------------------------------------------------

}   // namespace
