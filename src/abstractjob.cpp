#include <QDebug>

#include "abstractjob.h"

namespace thr {

//-----------------------------------------------------------------------------

AbstractJob::AbstractJob(QString qsName) : QObject()
{
    setName(qsName);
    m_iError = 0;
    m_bStop = false;
    m_bFinished = false;
    m_bSpawned = false;
    m_pThread = thread();
}

//-----------------------------------------------------------------------------

AbstractJob::~AbstractJob()
{   }

//-----------------------------------------------------------------------------

QString AbstractJob::errorText(int iErr) const
{
    Q_UNUSED(iErr);
    return tr("Unknown error");
}

//-----------------------------------------------------------------------------

bool AbstractJob::canStart() const
{
    while ((m_vspDependency.count() > 0) && (m_vspDependency.first()->isFinished() == true)) {
        m_vspDependency.removeFirst();
    }
    return (m_vspDependency.count() == 0);
}

//-----------------------------------------------------------------------------

void AbstractJob::addDependency(QSharedPointer<AbstractJob> spJob)
{
    if (spJob.data() != 0) {
        m_vspDependency.append(spJob);
    }
}

//-----------------------------------------------------------------------------

void AbstractJob::cleanup()
{
    if ((m_iError == 0) && (m_bStop == false)) {
        m_bFinished = true;
    }
}

//-----------------------------------------------------------------------------

void AbstractJob::exec()
{
    m_bStop = false;
    m_iError = 0;
    process();
    release();
    if (m_iError != 0) {
        emit signalError();
    }   else if (m_bStop == true) {
        emit signalStopped();
    }   else {
        emit signalFinished();
    }
}

//-----------------------------------------------------------------------------

void AbstractJob::stop()
{
    m_bStop = true;
}

//-----------------------------------------------------------------------------

void AbstractJob::release()
{
    moveToThread(m_pThread);
}

//-----------------------------------------------------------------------------

void AbstractJob::reportError(int iErr)
{
    m_iError = iErr;
}

//-----------------------------------------------------------------------------

}   // namespace

