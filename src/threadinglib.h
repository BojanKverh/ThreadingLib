#ifndef THREADINGLIB_H
#define THREADINGLIB_H

#include "threadinglib_global.h"

/**
 * @mainpage Threading library
 *
 * This library is intended to maximize the use of your computer processor(s).
 * It is meant to be used in cases, where there is a huge computing task to be done,
 * which can be divided into several smaller tasks that can be processed in parallel.
 * This library takes away the management of different threads and waiting for their
 * finish before assigning them the next job to process. You can also define dependencies between jobs in cases where some jobs have to be finished before some other job is started.
 *
 * The processing of the smaller tasks should be implemented in the process method of
 * the class derived from abstract class AbstractJob.
 *
 * In this library there are two
 * classes, which handle the processing of number of smaller tasks:
 * - class <b>JobManager</b>: if all the smaller tasks can be created at once, this is the
 *   preferred class to use. Create every small task needed to complete the
 *   computing and add it to JobManager via
 *   appendJob() method. Then call JobManager::start() method and JobManager will take
 *   care of the rest. When all the tasks are processed, JobManager will emit
 *   signal signalFinished.
 * - class <b>AbstractSessionManager</b>: if creating all the smaller tasks at once exceeds
 *   your system memory or some other system resource, you have to divide tasks into
 *   sessions. AbstractSessionManager is an abstract class, which has to be derived and its
 *   methods sessionCount() and initNextSession() have to be implemented. Method
 *   sessionCount should return the total number of sessions the tasks are divided into.
 *   Method initNextSession should create each small task belonging to the current
 *   session and add them into the internal JobManager object. Calling
 *   AbstractSessionManager::start() method will initialize first session and process
 *   all the tasks in it, then it will delete the tasks from first session,
 *   initialize second session and process all the tasks in it and so on until all
 *   the sessions are completed or the processing is stopped by calling the stop()
 *   method or too many errors occured during one session.
 */

class THREADINGLIBSHARED_EXPORT ThreadingLib
{

public:
    ThreadingLib();
};

#endif // THREADINGLIB_H
