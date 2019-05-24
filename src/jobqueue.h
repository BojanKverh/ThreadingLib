#ifndef JOBQUEUE_H
#define JOBQUEUE_H

/************************************************************************************
 *                                                                                  *
 *  Project:     ThreadingLib                                                       *
 *  File:        jobqueue.h                                                         *
 *  Class:       JobQueue                                                           *
 *  Author:      Bojan Kverh                                                        *
 *  License:     LGPL                                                               *
 *                                                                                  *
 ************************************************************************************/

#include <QVector>
#include <QMutex>
#include <QSharedPointer>

#include "abstractjob.h"

namespace thr {

/**
 * @brief The JobQueue class. This class can be used to process a number of jobs,
 * which have to be executed sequentially one after another in a
 * single separate thread
 *
 * @details This class is useful, if there are a number of jobs, which need to be processed
 * sequentially one after another in a separate thread. To insert a job into the
 * list of jobs, use the append() method. Calling start() method will start processing
 * jobs, starting with the first. After each job has finished, the next queued job
 * will be processed until there are no more queued jobs or until the error
 * occurs while processing a job. <br/><br/>
 * If all jobs were processed successfully, signal signalFinished() will be emitted.
 * If an error occur while processing a job, signal signalError will be emitted and
 * other queued jobs will not be processed. <br/><br/>
 * If a caller called stop() method, the
 * current job will be processed till the end, but the remaining queued jobs
 * will not be processed. After the current job is processed while the stop flag is
 * set to true, the process() method will exit and signal signalStop() will be
 * emitted.<br/><br/>
 * If auto delete flag is set to true with setAutoDelete() method, destructor and
 * clear() method will physically delete all jobs currently held by this queue from
 * the memory. The default value of the auto delete flag is false. <br/><br/>
 * This class is derived from AbstractJob, so it can be assigned to JobManager for
 * processing
 */
class JobQueue : public AbstractJob
{
    Q_OBJECT

public:
    /**
     * @brief JobQueue. Default constructor
     */
    JobQueue();
    /**
     * @brief ~JobQueue. Destructor
     */
    virtual ~JobQueue();
    /**
     * @brief append. Appends the given job to the job queue
     * @param pJob. Pointer to the abstract job object.
     */
    void append(AbstractJob* pJob);
    /**
     * @brief clear. Deletes all the jobs from queue
     */
    void clear();

    /**
     * @brief jobCount. Returns the number of jobs in the queue
     * @return number of jobs in the queue
     */
    int jobCount() const
    {   return m_vJobs.count(); }

    /**
     * @brief progress. Returns the progress in [%]
     * @return progress in [%]
     */
    int progress() const;

protected:
    /**
     * @brief process. Processes all the queued jobs
     */
    void process();

protected:
    /**
     * @brief m_mutex. Synchronization object
     */
    QMutex m_mutex;
    /**
     * @brief m_vJobs. Vector of shared pointers to jobs
     */
    QVector<QSharedPointer<AbstractJob> > m_vJobs;
    /**
     * @brief m_iCurrent. Current processing job index
     */
    int m_iCurrent;
};

}   // namespace

#endif // JOBQUEUE_H
