#include <QtTest>
#include <QDebug>

#include "sessionmanager.h"

//-----------------------------------------------------------------------------

SessionManager::SessionManager(int iT) : thr::AbstractSessionManager(iT)
{
    connect(this, SIGNAL(signalFinished()), this, SLOT(slotFinished()));
}

//-----------------------------------------------------------------------------

void SessionManager::initNextSession()
{
    int iN = 0;
    if (m_iSessionIndex == 0) {
        iN = 50;
    }   else if (m_iSessionIndex == 1) {
        iN = 100;
    }   else if (m_iSessionIndex == 2) {
        iN = 200;
    }   else {
        qCritical() << "SessionManager -> session index should be in [0-2]!";
        return;
    }

    for (int i = 0; i < iN; ++i) {
        appendJob(new JobRandom);
    }
}

//-----------------------------------------------------------------------------

void SessionManager::handleFinished()
{
    qDebug() << "*** Session" << m_iSessionIndex << "finished" << m_jm.finishedCount()
             << m_jm.jobCount();
    AbstractSessionManager::handleFinished();
}

//-----------------------------------------------------------------------------

void SessionManager::slotFinished()
{
    qDebug() << "SessionManager: all processing finished!";
}

//-----------------------------------------------------------------------------

