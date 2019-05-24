#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <stdlib.h>

#include "abstractsessionmanager.h"

#define NUM_MAX             1000
#define NUM_F               562

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

class SessionManager : public thr::AbstractSessionManager
{
    Q_OBJECT

public:
    SessionManager(int iT = 0);

    int sessionCount() const
    {   return 3; }

protected:
    void initNextSession();

protected slots:
    void handleFinished();
    void slotFinished();
};

#endif // SESSIONMANAGER_H
