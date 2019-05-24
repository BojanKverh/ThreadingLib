#ifndef ABSTRACTSESSIONMANAGER_H
#define ABSTRACTSESSIONMANAGER_H

/************************************************************************************
 *                                                                                  *
 *  Project:     ThreadingLib                                                       *
 *  File:        abstractsessionmanager.h                                           *
 *  Class:       AbstractSessionManager                                             *
 *  Author:      Bojan Kverh                                                        *
 *  License:     LGPL                                                               *
 *                                                                                  *
 ************************************************************************************/

#include <QMutex>

#include "jobmanager.h"

namespace thr {

/**
 * @brief The AbstractSessionManager class. This is an abstract base class for session
 * manager classes.
 *
 * @details Reimplement this class if you need lots of jobs being processed
 * in parallel in several sessions. Every job in a session has to be processed
 * until successful finish or an error before the next session can be started. <br/><br/>
 * In the derived class, two methods have to be reimplemented. Method sessionCount()
 * should be reimplemented to return the number of sessions. Method initNextSession()
 * should be reimplemented to populate job manager m_jm with the proper jobs, depending
 * on the session index. Member m_iSessionIndex contains the index of the current session,
 * starting with 0. After each session is finished, this counter is increased by 1. When it
 * reaches sessionCount(), the processing is finished and signalFinished() is emitted. <br/><br/>
 * User can reimplement the allowErrors() method to return the number of allowed
 * errors in jobs for current session. If the number of errors in a session exceeds
 * the number of allowed errors for this session, the processing will be stopped and
 * the signalError will be emitted. The default number of allowed errors for every
 * session is 0.
 *
 * Example:
 * @code
#define NUM_MAX             1000
#define NUM_F               562

// this job generates random numbers until a certain value is obtained
class JobRandom : public thr::AbstractJob
{
    Q_OBJECT

public:
    JobRandom()
    {   }

    void process()
    {
        while (next() != NUM_F)
        {   }
    }

private:
    int next() const
    {
        return 1 + (rand() % NUM_MAX);
    }
};
 * @endcode
 * <br/>
 * The AbstractSessionManager reimplementation:
 * @code
class SessionManager : public thr::AbstractSessionManager
{
    Q_OBJECT

public:
    SessionManager()
    {
        connect(this, SIGNAL(signalFinished()), this, SLOT(slotFinished()));
    }

    int sessionCount() const
    {   return 3; }

protected:
    void initNextSession()
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
            m_jm.appendJob(new JobRandom);
        }
    }

protected slots:
    void handleFinished()
    {
        qDebug() << "*** Session" << m_iSessionIndex << "finished" << m_jm.finishedCount()
                 << m_jm.jobCount();
        AbstractSessionManager::handleFinished();
    }

    void SessionManager::slotFinished()
    {
        qDebug() << "SessionManager: all processing finished!";
    }
};
 * @endcode
 * The usage is quite simple:
 * @code
    SessionManager sm;
    sm.start();
    while (sm.isRunning() == true) {
        QCoreApplication::instance()->processEvents();
    }
    qDebug() << "All sessions finished";
 * @endcode
 * Instead of waiting in the event processing loop until the session manager is finished,
 * one can also connect to the session manager signalFinished, which will be emitted
 * after all sessions are finished.
 */
class AbstractSessionManager : public QObject
{
    Q_OBJECT

protected:
    /**
     * @brief The Status enum. This internal structure holds the status of the
     * current (or latest) operation performed by the AbstractSessionManager
     */
    enum Status {
        sRunning,               //!< the session manager is processing the current session jobs
        sPaused,                //!< processing is paused because of next session initialization
        sFinished,              //!< all sessions are finished
        sStopped,               //!< processing was stopped from outside
        sError,                 //!< the number of jobs ended with error exceeded the allowed number of errors for current session
    };
public:
    /**
     * @brief AbstractSessionManager.
     * @param iThreads. Number of threads to be used in sessions. If this parameter is
     * less or equal to 0, the ideal number of threads for underlying CPU will be used
     * @param pParent. Pointer to the parent object
     */
    AbstractSessionManager(int iThreads = 0, QObject* pParent = 0);
    /**
     * @brief ~AbstractSessionManager. Destructor
     */
    virtual ~AbstractSessionManager();
    /**
     * @brief sessionCount. Reimplement this method to return the number of sessions
     * @return number of sessions
     */
    virtual int sessionCount() const = 0;
    /**
     * @brief isRunning. Returns true, if session manager is still
     * running and false otherwise
     * @return true, if session manager is processing the session jobs
     * or is initializing the next session. Otherwise, it returns false
     */
    bool isRunning() const
    {   return (m_eStatus == sRunning) || (m_eStatus == sPaused); }
    /**
     * @brief isFinished. Returns true, if session manager successfully finished
     * processing all the sessions.
     * @return True, if session manager successfully finished processing all
     * the sessions. It will return false in all other cases, such as:
     * - session manager hasn't started yet
     * - too many errors occured during one session processing
     * - processing was stopped by calling the stop() method
     * - processing is still under way
     */
    bool isFinished() const
    {   return m_eStatus == sFinished; }
    /**
     * @brief currentSession. Returns the current session index
     * @return current session index
     */
    int currentSession() const
    {   return m_iSessionIndex; }
    /**
     * @brief setSessionTimeout. Sets the timeout, that will be enforced between one session
     * ending and the next session starting.
     * @param iTime. Timeout in [ms]
     */
    void setSessionTimeout(int iTime)
    {   m_iSessionTimeout = iTime; }
    /**
     * @brief finishedJobs. Returns the total number of finished jobs
     * @return total number of finished jobs
     */
    int finishedJobs() const
    {   return m_iFinished; }
    /**
     * @brief appendJob. Appends the job to the current session
     * @param pJob. Pointer to the new job, which will be added to the
     * current session. AbstractSessionManager takes ownership of this
     * object!
     */
    void appendJob(thr::AbstractJob* pJob);
    /**
     * @brief addThreads. Adds iT threads for execution to the internal job manager
     * object. This method can be called even in the middle of processing.
     * @param iT number of threads, that will be added to the current threads
     */
    void addThreads(int iT);
    /**
     * @brief threadsRunningCount. Returns the number of threads that
     * are currently running
     * @return number of threads that are currently running
     */
    int threadsRunningCount() const;

public slots:
    /**
     * @brief start. Starts executing the first session (session index 0)
     * @return true, if the first session was successfully started and
     * false otherwise. The latter happens if the session count is 0 or
     * if the session manager is already running or if the internal job
     * manager could not be started
     */
    virtual bool start();
    /**
     * @brief stop. Stops the job manager executing the current session
     */
    virtual void stop();

signals:
    /**
     * @brief signalFinished. Emitted when all the jobs in all sessions are finished
     */
    void signalFinished();
    /**
     * @brief signalSessionFinished. Emitted every time a session is finished. Session
     * is finished when all its jobs are processed
     * @param iInd. Index of the session that just finished
     */
    void signalSessionFinished(int iInd);
    /**
     * @brief signalError. Emitted when the number of errors exceeds the number
     * of allowed errors in current session
     * @param iInd. Index of the session, where the error occured
     * @param eJME. Describes the error, which occured during job processing
     */
    void signalError(int iInd, thr::JobManagerError eJME);
    /**
     * @brief signalStopped. Emitted when job processing was stopped from outside
     * @param iInd. Index of the session, which was interrupted
     */
    void signalStopped(int iInd);
    /**
     * @brief signalProgress. This signal reports the amount of work done
     * @param iPer. Total amount of work done in [%]
     */
    void signalProgress(int iPer);

protected:
    /**
     * @brief initNextSession. Reimplement this method to initialize the next session.
     * This method will be called automatically by the session manager before each
     * session is started. Current session index is held in the m_iSessionIndex member.
     * This method should fill the job manager with jobs, that should be processed in
     * current session
     */
    virtual void initNextSession() = 0;
    /**
     * @brief allowedErrors. This method should return a maximal number of allowed errors
     * for the current session.
     * @return maximal number of allowed errors for the current session
     */
    virtual int allowedErrors() const
    {   return 0; }

protected slots:
    /**
     * @brief handleFinished. This method is called whenever a session is finished.
     * It will emit signalSessionFinished signal and increase the session index.
     * If all the sessions are finished (when session index becomes equal to
     * sessionCount()), signalFinished signal will also be emitted.
     */
    virtual void handleFinished();
    /**
     * @brief handleError. This method will be called, when an error occured within the
     * job manager. It will emit the signalError signal
     * @param eJME. Describes the error, which occured during job processing
     */
    virtual void handleError(thr::JobManagerError eJME);
    /**
     * @brief handleStopped. This method will be called when the processing was stopped
     * from outside. It will emit the signalStopped signal
     */
    virtual void handleStopped();
    /**
     * @brief handleProgress. This method will be called whenever a progress is reported
     * by the job manager. It will emit the signalProgress signal with proper percentage
     * of the amount of work done.
     * @param iPer. Current session percentage of the work done
     */
    virtual void handleProgress(int iPer);
    /**
     * @brief handleJobFinished. Increases the number of jobs finished
     */
    virtual void handleJobFinished();

    /**
     * @brief startNextSession. Starts the next session
     */
    virtual void startNextSession();

protected:
    /**
     * @brief m_eStatus. This variable denotes the current status of the object
     */
    Status m_eStatus;
    /**
     * @brief m_jm. Job manager object
     */
    JobManager m_jm;
    /**
     * @brief m_iSessionIndex. Holds the session index. Session index starts at 0 and
     * increases after every session that is finished.
     */
    int m_iSessionIndex;
    /**
     * @brief m_iSessionTimeout. Timeout between one session finish and next
     * session start in [ms]. The default value is 0.
     */
    int m_iSessionTimeout;
    /**
     * @brief m_iFinished. Contains the number of finished jobs
     */
    int m_iFinished;
};

}   // namespace

#endif // ABSTRACTSESSIONMANAGER_H
