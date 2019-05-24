#ifndef ABSTRACTJOB_H
#define ABSTRACTJOB_H

/************************************************************************************
 *                                                                                  *
 *  Project:     ThreadingLib                                                       *
 *  File:        abstractjob.h                                                      *
 *  Class:       AbstractJob                                                        *
 *  Author:      Bojan Kverh                                                        *
 *  License:     LGPL                                                               *
 *                                                                                  *
 ************************************************************************************/

#include <QObject>
#include <QThread>
#include <QVector>

#define CHECK_JOB_STOP() \
    if (m_bStop == true) {\
        return;\
    }

namespace thr {

/**
 * @brief The AbstractJob class. This is base class for all jobs, which are
 * supposed to be executed in by a JobManager in a separate thread.
 *
 * @details In order to create a proper job to be processed by the JobManager, user has
 * to create a class, derived from AbstractJob and implement its pure virtual
 * methods progress() and process(). <br/><br/>
 * Method progress() should return the amount of job done in percentage, while
 * process() should do the actual job processing. The process()
 * method should not be called directly. JobManager and AbstractSessionManager
 * will call AbstractJob::exec() method when appropriate and this
 * will call the AbstractJob's process() method and wait until it exits. <br/><br/>
 * If an error occurs during the processing, the process() method should set
 * the m_iError to a value greater than zero, indicating the type of the error and
 * exit. There is no need for the derived class to emit the signal signalError(),
 * because AbstractJob::exec() method will do that, if m_iError is greater than
 * zero. <br/><br/>
 * If the derived class processing should be interruptable, the process() method
 * of the derived class should check the stop flag using CHECK_JOB_STOP() macro as
 * often as feasible. This macro will exit
 * the method if stop flag is set to true. If processing was stopped by setting
 * the stop flag, AbstractJob::exec() method will emit signal
 * signalStopped(). <br/><br/>
 * After AbstractJob::exec() method regains control after the process() method exit,
 * it checks the value of m_iError and stop flag. If m_iError is greater than zero,
 * signal signalError() will be emitted. If stop flag is set to true, signal
 * signalStop() will be emitted. Only if m_iError is zero and stop flag is
 * set to false, the job is considered to be completed successfully.
 * Signal signalFinished() will be emitted to report successful job
 * completion. <br/><br/>
 * If a job should not be started by the JobManager until a number of other jobs
 * has finished first, all those jobs should be added to the list of dependencies
 * using addDependency() method. This allows the creation of complex dependency
 * patterns between different jobs. <br/><br/>
 *
 * The following example shows, how to implement a job, which can calculate
 * a sum of first m_uiMax positive integers in a brute force way in a JobManager's
 * separate thread:
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
 */
class AbstractJob : public QObject
{
    Q_OBJECT

    friend class JobQueue;
    friend class JobManager;

public:
    /**
     * @brief AbstractJob. Default constructor
     * @param qsName. Job name. It can be left empty, but setting it can be useful
     * when debugging
     */
    AbstractJob(QString qsName = "");
    /**
     * @brief ~AbstractJob. Destructor
     */
    virtual ~AbstractJob();

    /**
     * @brief name. Returns the job name
     * @return job name
     */
    QString name() const
    {   return m_qsName; }
    /**
     * @brief setName. Sets the job name
     * @param qsName. New job name
     */
    void setName(QString qsName)
    {   m_qsName = qsName; }
    /**
     * @brief progress. Reimplement this method to return the exact amount of
     * processing done, if necessary
     * @return amount of processing done in [%]
     */
    virtual int progress() const
    {
        if (m_bFinished == false)
            return 0;
        else
            return 100;
    }

    /**
     * @brief errorText. Reimplement this method to return error text for
     * given error code
     * @param iErr. Error code
     * @return error text for given error code
     */
    virtual QString errorText(int iErr) const;

    /**
     * @brief isStopped. Returns true, if the job was stopped and false otherwise
     * @return true, if the job was stopped and false otherwise
     */
    bool isStopped() const
    {   return m_bStop; }

    /**
     * @brief isError. Returns true, if the job was stopped because of an internal
     * error
     * @return true, if the job was stopped because of internal error and false
     * otherwise
     */
    bool isError() const
    {   return m_iError != 0; }
    /**
     * @brief errorCode. Returns the error code
     * @return error code. If the job finished successfully, it will return 0
     */
    int errorCode() const
    {   return m_iError; }

    /**
     * @brief isSpawned. Returns the value of the spawned flag
     * @return true, if the job was spawned from another job and false otherwise
     */
    bool isSpawned() const
    {   return m_bSpawned; }

    /**
     * @brief sourceThread. Returns pointer to the thread in which this object
     * was created
     * @return pointer to the thread in which this object was created
     */
    QThread* sourceThread() const
    {   return m_pThread; }

    /**
     * @brief canStart. This method first deletes all the dependencies, which
     * have already been processed, from the list of dependencies. If the list
     * of dependencies is empty, it will return true, otherwise it will return
     * false. The derived class can reimplement this method to check additional
     * conditions, but it should always first call AbstractJob::canStart() in
     * order to preserve the dependencies checking; if AbstractJob::canStart()
     * returns false, so should the canStart() in the derived class. Otherwise,
     * it can check additional conditions and return a proper value.
     * @return true, if the job is ready to be processed and false otherwise
     */
    virtual bool canStart() const;

    /**
     * @brief isFinished. Returns the finished flag
     * @return true, if the job was finished successfully and false if the error
     * occured or the job is not finished yet
     */
    bool isFinished() const
    {
        return m_bFinished;
    }

    /**
     * @brief addDependency. Adds new dependency to the vector of dependencies.
     * @param spJob. Shared pointer to the job, that has to be finished
     * before this job is started.
     */
    void addDependency(QSharedPointer<thr::AbstractJob> spJob);
    /**
     * @brief dependencyCount. Returns the number of dependencies
     * @return number of dependencies left to finish
     */
    int dependencyCount() const
    {   return m_vspDependency.count(); }

    /**
     * @brief cleanup. This method will be called when the job is finished. It can
     * be used to release some resources, which are not needed anymore after the
     * job is processed. If you reimplement this method in the derived class,
     * make sure you call AbstractJob::cleanup in the beginning of the
     * reimplemented method.
     */
    virtual void cleanup();

public slots:
    /**
     * @brief exec. Starts the processing. After the processing is finished, it will move
     * this object back into the thread it was created in. If an error occured during the
     * processing, signalError() will be emitted. If the processing was stopped from outside,
     * signalStopped() will be emitted. If processing finished successfully, signalFinished()
     * will be emitted.
     */
    virtual void exec();
    /**
     * @brief stop. Sets the stop flag. In order to stop the execution as soon
     * as possible after the stop() method has been called, use the CHECK_JOB_STOP
     * macro in process() method as often as feasible.
     */
    virtual void stop();

signals:
    /**
     * @brief signalFinished. This signal is emitted when the job has finished
     * processing successfully.
     */
    void signalFinished();
    /**
     * @brief signalStopped. This signal is emitted when the processing was stopped
     * from outside of this class
     */
    void signalStopped();
    /**
     * @brief signalError. This signal is meant to report errors, but when some error
     * occurs during job processing, do not emit this signal directly!
     * Instead, call the method reportError(int iErr), which will properly
     * set the error code and make sure that this signal will be emitted after
     * returning from process() method.
     */
    void signalError();

protected:
    /**
     * @brief process. Reimplement this method to do the processing.
     */
    virtual void process() = 0;
    /**
     * @brief nextSpawnedJob. This method can be reimplemented in cases when after job
     * processing is finished, there is a need to add more jobs to the job manager.
     * Calling this method should create the next job that
     * is spawned when this job is finished (if necessary) and return a pointer to it.
     * This method will be called by the JobManager's handleJobFinished method repeatedly
     * until it returns 0. It will be called before this job's cleanup method is called.
     * To see the example how to use this method properly, navigate to examples/qsort.
     * @return next job that is spawned when this job is finished. If no more jobs are
     * spawned, it should return 0.
     */
    virtual AbstractJob* nextSpawnedJob()
    {   return 0; }

protected slots:
    /**
     * @brief release. Releases the object from the current thread
     */
    virtual void release();
    /**
     * @brief reportError. Sets the error code to iErr
     * @param iErr. New error code
     */
    virtual void reportError(int iErr);

private:
    /**
     * @brief setSpawned. Sets the spawned flag to true
     */
    void setSpawned()
    {   m_bSpawned = true; }

protected:
    /**
     * @brief m_qsName. Job name
     */
    QString m_qsName;

    /**
     * @brief m_bStop. Stop flag
     */
    bool m_bStop;
    /**
     * @brief m_bFinished. This flag will be set to true, when job is finished
     * successfully.
     */
    bool m_bFinished;
    /**
     * @brief m_vspDependency. This vector contains shared pointers to jobs, that
     * this job is dependent on. This job cannot be started until all the jobs from
     * this vector are finished successfully.
     */
    mutable QVector<QSharedPointer<AbstractJob> > m_vspDependency;

private:
    /**
     * @brief m_iError. Error code. If an error occurs, set this variable to
     * a value greater than zero using the reportError() method. Default
     * value is 0, which means no error.
     */
    int m_iError;

    /**
     * @brief m_pThread. Pointer to the thread, where the object was created
     */
    QThread* m_pThread;
    /**
     * @brief m_bSpawned. Spawned flag, which is set to true, if the job
     * was spawned from another job
     */
    bool m_bSpawned;
};

}   // namespace

#endif // ABSTRACTJOB_H
