#ifndef THREAD_H
#define THREAD_H

#include <QThread>
#include <QSharedPointer>

#include "abstractjob.h"

namespace thr {

/**
 * @brief The Thread class. This class is used to execute scheduled jobs. It
 * is used by JobManager for putting the job into a different thread
 */
class Thread : public QThread
{
    Q_OBJECT

public:
    /**
     * @brief Thread. Default constructor
     */
    Thread();
    /**
     * @brief ~Thread. Destructor
     */
    virtual ~Thread();
    /**
     * @brief jobIndex. Returns the index of a job, which is processing in this thread
     * @return index of a job, which is processing in this thread
     */
    int jobIndex() const
    {   return m_iJobIndex; }
    /**
     * @brief start. Starts processing given job
     * @param iJobIndex. Index of job in the jobs vector
     * @param spJob. Pointer to the job object to process. This object does not take
     * ownership of the job!
     */
    void start(int iJobIndex, QSharedPointer<AbstractJob> spJob);

private:
    /**
     * @brief m_iJobIndex. Index of the current job in the jobs vector
     */
    int m_iJobIndex;
    /**
     * @brief m_spJob. Shared pointer to the processing job object
     */
    QSharedPointer<AbstractJob> m_spJob;
};

}   // namespace

#endif // THREAD_H
