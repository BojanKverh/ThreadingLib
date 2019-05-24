#include <QDebug>

#include "abstractsessionmanager.h"

namespace thr {

//-----------------------------------------------------------------------------

AbstractSessionManager::AbstractSessionManager(int iThreads, QObject* pParent) :
    QObject(pParent),
    m_jm(iThreads)
{
    m_iSessionIndex = -1;
    m_iSessionTimeout = 0;
    m_eStatus = sFinished;

    m_jm.setReportJobFinish(true);
    connect(&m_jm, SIGNAL(signalFinished()), this, SLOT(handleFinished()));
    connect(&m_jm, SIGNAL(signalError(thr::JobManagerError)), this, SLOT(handleError(thr::JobManagerError)));
    connect(&m_jm, SIGNAL(signalStopped()), this, SLOT(handleStopped()));
    connect(&m_jm, SIGNAL(signalProgress(int)), this, SLOT(handleProgress(int)));
    connect(&m_jm, SIGNAL(signalJobFinished(QSharedPointer<thr::AbstractJob>)), this, SLOT(handleJobFinished()));
}

//-----------------------------------------------------------------------------

AbstractSessionManager::~AbstractSessionManager()
{
    m_jm.clear();
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::appendJob(AbstractJob* pJob)
{
    m_jm.appendJob(pJob);
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::addThreads(int iT)
{
    m_jm.addThreads(iT);
}

//-----------------------------------------------------------------------------

int AbstractSessionManager::threadsRunningCount() const
{
    return m_jm.threadsRunningCount();
}

//-----------------------------------------------------------------------------

bool AbstractSessionManager::start()
{
    if (isRunning() == true) {
        qWarning() << "Cannot start session manager, when it is already running!";
        return false;
    }

    if (sessionCount() == 0) {
        qWarning() << "No sessions to process!";
        m_eStatus = sFinished;
        emit signalFinished();
        return true;
    }

    m_iSessionIndex = 0;
    m_iFinished = 0;
    m_eStatus = sPaused;
    startNextSession();
    return m_eStatus == sRunning;
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::stop()
{
    if (m_jm.isRunning() == true) {
        m_jm.stop();
    }   else {
        handleStopped();
    }
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::handleFinished()
{
    if (m_eStatus != sRunning) {
        emit signalError(m_iSessionIndex, thr::jmeImplementationError);
        m_eStatus = sError;
        return;
    }

    m_eStatus = sPaused;
    emit signalSessionFinished(m_iSessionIndex);
    ++m_iSessionIndex;
    if (m_iSessionIndex < sessionCount()) {
        QTimer::singleShot(m_iSessionTimeout, this, SLOT(startNextSession()));
    }   else {
        m_eStatus = sFinished;
        emit signalFinished();
    }
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::handleError(thr::JobManagerError eJME)
{
    m_eStatus = sError;
    emit signalError(m_iSessionIndex, eJME);
    m_iSessionIndex = -1;
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::handleStopped()
{
    m_eStatus = sStopped;
    emit signalStopped(m_iSessionIndex);
    m_iSessionIndex = -1;
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::handleProgress(int iPer)
{
    int iTotalPer = (100*m_iSessionIndex + iPer)/sessionCount();
    emit signalProgress(iTotalPer);
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::handleJobFinished()
{
    ++m_iFinished;
}

//-----------------------------------------------------------------------------

void AbstractSessionManager::startNextSession()
{
    if (m_jm.isStopped() == true) {
        handleStopped();
        return;
    }
    m_jm.clear();
    m_jm.setAllowedErrors(allowedErrors());
    initNextSession();
    m_eStatus = sRunning;
    if (m_jm.start() == false) {
        m_eStatus = sError;
        emit signalError(m_iSessionIndex, jmeCouldNotStart);
    }
}

//-----------------------------------------------------------------------------

}   // namespace

