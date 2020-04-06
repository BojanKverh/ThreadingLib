#ifndef JOBMANAGER_H
#define JOBMANAGER_H

/************************************************************************************
 *                                                                                  *
 *  Project:     ThreadingLib                                                       *
 *  File:        jobmanager.h                                                       *
 *  Class:       JobManager                                                         *
 *  Author:      Bojan Kverh                                                        *
 *  License:     LGPL                                                               *
 *                                                                                  *
 ************************************************************************************/

#include <QThread>
#include <QVector>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QSharedPointer>

#include "abstractjob.h"
#include "thread.h"

namespace thr {

/**
 * @brief The JobManagerError enum. This enum describes an error, which occured
 * during job processing with JobManager object
 */
enum JobManagerError {
    jmeNoError,                     //!< no error
    jmeTooManyErrors,               //!< number of failed jobs exceeds the number of allowed errors
    jmeNoJobReady,                  //!< no queued job is ready for processing
    jmeCouldNotStart,               //!< could not start job manager

    jmeImplementationError = 900,   //!< implementation error, if this happens, please send us a bug report

    jmeUserDefined = 1000,          //!< values greater than this are reserved for user defined errors
};

/**
 * @brief The JobManager class. This class is used to process several jobs at once,
 * each one in a separate thread.
 *
 * @details JobManager will use a maximal allowed number of threads
 * to process jobs. Jobs can be assigned to JobManager via append() method. Every job
 * assigned to JobManager has to inherit from AbstractJob class, implementing
 * its pure virtual methods. Processing of jobs can be started via start() method.
 * Each thread can process one job at the time. If the number of assigned jobs
 * exceeds the number of available threads, as soon as one thread finishes processing
 * its current job, it will be assigned the next queued job, which can be started,
 * and it will start processing again. When thread is finished processing its current
 * job and there are still queued jobs left, but none of them can be started,
 * JobManager will emit signal signalError() with jmeNoJobReady parameter.<br/><br/>
 *
 * Even though there is no limitation (besides the physical memory available) on the number
 * of jobs assigned to the job manager, one has to be careful not to exaggerate, because
 * AbstractJob class is derived from QObject and QObject creation and removal from memory
 * are time consuming operations. So if hundreds of thousands or even millions of simple jobs
 * are assigned to job manager, the QObject overhead will surely outdo the benefits of
 * multi-threaded processing. <br/><br/>
 *
 * A caller can set the number of allowed errors (default is 0). If at any point,
 * the number of jobs, which failed to be processed correctly, exceeds the
 * number of allowed errors, JobManager will not process any other queued jobs and
 * will wait until all the threads finish processing and then it will emit
 * signal signalError() with jmeTooManyErrors parameter. If JobManager
 * finishes processing and the number of failed jobs does not exceed the number
 * of allowed errors, signal signalFinished() is emitted.<br/><br/>
 * User can stop the JobManager processing by calling stop() method. The JobManager
 * will then wait for all the threads to finish processing and will not start
 * processing the remaining queued jobs. In this case, after all threads are finished,
 * JobManager will emit signal signalStopped(). <br/><br/>
 *
 * JobManager can report progress, which is calculated as a percentage of finished
 * jobs in regard to the total number of jobs scheduled. User can set the interval
 * at which the progress is reported via signal signalProcess() with
 * setProgressReportTimeout method. By default, JobManager does not report
 * progress. <br/><br/>
 *
 * JobManager takes ownership of all the jobs appended to it via append() method and
 * all the jobs that are spawned from the previously finished jobs, storing
 * them with the shared pointers. Every job will be deleted by JobManager destructor
 * or the clear() method, unless it was retrieved from job() method and stored by
 * another shared pointer. <br/><br/>
 *
 * Additional threads can be added to JobManager even during the job processing with
 * addThreads() method. <br/><br/>
 *
 * @code
class TestJob : public thr::AbstractJob
{
    Q_OBJECT

public:
    TestJob(unsigned int uiMax = 100) : thr::AbstractJob()
    {
        m_uiCount = 0;
        setMax(uiMax);
    }

    void setMax(unsigned int uiMax)
    {   m_uiMax = uiMax; }

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
 * @endcode
 * <br/>
 * @code
 *
    thr::JobManager jm(3);
    TestJob* pJob1 = new TestJob(100);
    TestJob* pJob2 = new TestJob(200);
    TestJob* pJob3 = new TestJob(300);

    jm.appendJob(pJob1);
    jm.appendJob(pJob2);
    jm.appendJob(pJob3);

    jm.start();
    while (jm.isRunning() == true) {
        QCoreApplication::instance()->processEvents();
    }

    qDebug() << "Sums" << pJob1->sum() << pJob2->sum() << pJob3->sum();

 * @endcode
 * Since JobManager takes ownership of the jobs that are appended to it, pJob1, pJob2
 * and pJob3 will be deleted in jm destructor, so the user should not delete them
 * himself
 */
class JobManager : public QObject
{
    Q_OBJECT

    /**
     * @brief The Status enum. This internal structure holds the status of the
     * current (or latest) operation performed by the JobManager
     */
    enum Status {
        sRunning,               //!< processing is still going on
        sFinished,              //!< all jobs are finished
        sStopped,               //!< processing was stopped from outside
        sError,                 //!< the number of jobs ended with error exceeded the allowed number of errors
    };

public:
    /**
     * @brief JobManager. Constructor
     * @param iThreads maximum number of threads for simultaneous processing. If
     * this parameter is less or equal to 0, it will use the ideal number of threads
     * for the underlying CPU.
     * @param pParent pointer to the parent object
     */
    JobManager(int iThreads = 0, QObject* pParent = 0);
    /**
     * @brief ~JobManager. Destructor
     */
    virtual ~JobManager();

    /**
     * @brief setReportJobFinish. Sets the report job finish flag to
     * bReportJobFinish. If this flag is set to true, when a job is finished,
     * JobManager will report this event by emitting signal
     * signalJobFinished().
     * @param bReportJobFinish new value of the report job finish flag.
     */
    void setReportJobFinish(bool bReportJobFinish)
    {   m_bReportJobFinish = bReportJobFinish; }

    /**
     * @brief isReportJobFinish. Returns the report job finish flag.
     * If this flag is set to true, when a job is finished, JobManager will
     * report this event by emitting signal signalJobFinished()
     * @return report job finish flag
     */
    bool isReportJobFinish() const
    {   return m_bReportJobFinish; }

    /**
     * @brief job. Returns the shared pointer to the i-th job
     * @param i. Job index
     * @return read only shared pointer to the i-th job object
     */
    const QSharedPointer<AbstractJob> job(int i) const
    {   return m_vspJobs.at(i); }

    /**
     * @brief appendJob. Appends the given job to the vector of jobs to be
     * processed. JobManager always takes
     * ownership of this job object and deletes it in destructor
     * or when clear() method is called, unless it was retrieved from job() method
     * and stored by another shared pointer. Before being appended to the
     * JobManager, a job object has to be created on the heap!
     * @code
     * class TestJob : public thr::AbstractJob
     * {
     *     public:
     *        void process()
     *        {
     *          // implement the processing here
     *        }
     * };
     *
     * JobManager jm;
     * TestJob job;
     * jm.append(&job);             // wrong, will crash the application
     * jm.append(new TestJob);      // correct
     * @endcode
     * @param pJob pointer to the job object.
     */
    void appendJob(AbstractJob* pJob);
    /**
     * @brief clear. Deletes all the jobs in the job queue and makes the queue empty.
     */
    void clear();
    /**
     * @brief setAllowedErrors. Sets the number of jobs, that are allowed to
     * finish processing with an error.
     * @param iN new number of jobs, that are allowed to finish processing with
     * an error. If this is set to negative value, processing will continue
     * regardless of the number of jobs, which processing resulted in an error
     */
    void setAllowedErrors(int iN);

    /**
     * @brief allowedErrors. Returns the number of allowed errors
     * @return number of allowed errors
     */
    int allowedErrors() const
    {   return m_iAllowedErrors; }

    /**
     * @brief setProgressReportTimeout. Sets the time interval at which the progress
     * will be report. The default timeout is set to 0, meaning that no progress
     * is reported, until caller calls this method with a value greater than zero.
     * @param iMS time interval in [ms].  If this is set to 0, no progress will be
     * reported at all
     */
    void setProgressReportTimeout(int iMS);
    /**
     * @brief setThreads. Sets the number of threads. This method should only be called
     * when JobManager is idle. If the method is called, when JobManager is running,
     * it will do nothing.
     * @param iT new number of threads
     */
    void setThreads(int iT);
    /**
     * @brief addThreads. Adds iT threads for execution. This method can be called even
     * in the middle of processing.
     * @param iT number of threads, that will be added to the current threads
     */
    void addThreads(int iT);
    /**
     * @brief threadsRunningCount. Returns the number of threads, which are
     * actually running
     * @return number of threads, which are actually running
     */
    int threadsRunningCount() const;

    /**
     * @brief threadCount. Returns the number of threads
     * @return number of threads
     */
    int threadCount() const
    {   return m_vThreads.count(); }
    /**
     * @brief isIdle. Returns true, if this class is not processing jobs at the
     * moment. It returns the opposite value as the isRunning() method
     * @return true, if this class is not processing jobs at the moment for whatever
     * reason and false otherwise.
     */
    bool isIdle() const
    {   return m_eStatus != sRunning; }

    /**
     * @brief isFinished. Returns true, if job manager successfully finished
     * processing.
     * @return True, if job manager successfully finished processing. It returns
     * false in all other cases, such as:
     * - if job manager didn't start
     * - if too many errors occured during processing
     * - if the processing was stopped by calling the stop() method
     * - if the processing is still under way
     */
    bool isFinished() const
    {   return m_eStatus == sFinished; }

    /**
     * @brief jobCount. Returns the total number of queued jobs
     * @return number of queued jobs
     */
    int jobCount() const
    {   return m_vspJobs.count(); }
    /**
     * @brief finishedCount. Returns the number of finished jobs
     * @return number of finished jobs
     */
    int finishedCount() const
    {   return m_iFinished; }

    /**
     * @brief isRunning. Returns true, if at least one job is still running.
     * It returns the opposite value of the isIdle() method
     * @return true, if at least one job is running and false otherwise
     */
    bool isRunning() const
    {   return m_eStatus == sRunning; }
    /**
     * @brief isStopped. Returns true, if the processing was stopped.
     * @return true, if the processing was stopped from outside and false otherwise
     */
    bool isStopped() const
    {   return m_bStop; }

public slots:
    /**
     * @brief start. Starts processing the jobs
     * @return true, if all the threads required were started and false otherwise
     */
    bool start();

    /**
     * @brief stop. Stops all the processing threads
     */
    void stop();

signals:
    /**
     * @brief signalFinished. Emitted when all jobs are finished
     */
    void signalFinished();
    /**
     * @brief signalJobFinished. If report job finish flag is set to true,
     * JobManager will emit this signal every time one job has finished the processing
     * successfully.
     * If the report job finish flag is set to false, this signal will not be emitted
     * at all.
     * @param pJob pointer to the job, which processing just finished successfully
     */
    void signalJobFinished(QSharedPointer<thr::AbstractJob> spJob);
    /**
     * @brief signalError. Emitted when the number of errors exceeds the number
     * of allowed errors and all threads became idle afterwards
     * @param eJME describes the error, which occured during job processing
     */
    void signalError(thr::JobManagerError eJME);
    /**
     * @brief signalStopped. Emitted when the job processing have been stopped
     * from outside and all threads became idle afterwards
     */
    void signalStopped();
    /**
     * @brief signalProgress. This signal will be emitted in regular time intervals
     * to report progress
     * @param iPer percentage of finished jobs
     */
    void signalProgress(int iPer);

protected slots:
    /**
     * @brief handleJobFinished. This method will be called after one job is
     * finished. If there are more jobs to be processed, it will start processing
     * the next queued job
     */
    void handleJobFinished();
    /**
     * @brief reportProgress. Reports the progress by emitting signalProgress signal.
     * Progress is reported as a percentage of finished jobs in regard to the
     * total number of jobs scheduled for processing
     */
    virtual void reportProgress();

private:
    /**
     * @brief checkNext. Checks if next job can be started and if yes, it will
     * start it
     * @param iThrIndex index of an idle thread, where new job will be started
     */
    void checkNext();
    /**
     * @brief startNext. Starts the next job
     */
    virtual void startNext();

    /**
     * @brief allocateThreads. Allocate the number of threads
     * @param iT new number of threads
     */
    void allocateThreads(int iT);
    /**
     * @brief handleError. Reports the processing error by emitting signal
     * signalError, if necessary. The errors
     * are reported if m_eError is not equal to jmeNoError and the number
     * of running threads is zero
     * @return true, if m_eError is not equal to jmeNoError and false otherwise
     */
    bool handleError();
    /**
     * @brief appendJobUnsafe. This will append a job to the vector of jobs without
     * locking mutex
     * @param spJob. Pointer to the job object to append
     */
    void appendJobUnsafe(QSharedPointer<AbstractJob> spJob);

protected:
    /**
     * @brief m_vThreads. Vector of threads
     */
    QVector<QSharedPointer<Thread> > m_vThreads;
    /**
     * @brief m_quIdle. Queue of idle threads
     */
    QQueue<QSharedPointer<Thread> > m_quIdle;
    /**
     * @brief m_vspJobs. Vector of shared pointers to jobs to process
     */
    QVector<QSharedPointer<AbstractJob> > m_vspJobs;
    /**
     * @brief m_quWaiting. Vector of job indices waiting to be started
     */
    QQueue<int> m_quWaiting;
    /**
     * @brief m_timer. Timer for reporting progress
     */
    QTimer m_timer;
    /**
     * @brief m_iStarted. Number of started jobs
     */
    int m_iStarted;
    /**
     * @brief m_iRunning. Number of currently running threads
     */
    int m_iRunning;
    /**
     * @brief m_iFinished. Number of finished jobs
     */
    int m_iFinished;
    /**
     * @brief m_mutex. Synchronization object
     */
    mutable QMutex m_mutex;
    /**
     * @brief m_iAllowedErrors. Number of allowed errors. When the number of
     * jobs exceeds the number of allowed errors, the processing will not continue.
     * If this value is set to negative value, the processing will continue
     * regardless of the number of jobs, which processing resulted in an error.
     * The default value is 0, so the processing will not continue when one
     * job processing resulted in an error.
     */
    int m_iAllowedErrors;
    /**
     * @brief m_iErrors. Number of jobs, which processing resulted in an error
     */
    int m_iErrors;
    /**
     * @brief m_eStatus. This variable denotes the current status of the object
     */
    Status m_eStatus;
    /**
     * @brief m_eError. Contains the code of the last error occured during processing
     */
    JobManagerError m_eError;

private:
    /**
     * @brief m_bStop. This flag indicates, whether there have been a request to
     * stop processing.
     */
    bool m_bStop;
    /**
     * @brief m_bReportJobFinish. If this flag is set to true, the JobManager
     * will report every finished job by emitting signal signalJobFinished().
     * The default value of this flag is false.
     */
    bool m_bReportJobFinish;
};

}   // namespace

#endif // JOBMANAGER_H
