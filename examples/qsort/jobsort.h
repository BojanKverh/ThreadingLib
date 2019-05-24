#ifndef JOBSORT_H
#define JOBSORT_H

#include <QDebug>

#include "abstractjob.h"

#define QS_LIMIT        150

/**
 * @class JobSort. This class is used to sort items. Item class should have operators = and
 * < defined
 */
class JobSort : public thr::AbstractJob
{
    Q_OBJECT

public:
    /**
     * @brief JobSort. Constructor
     * @param paElem. Pointer to the array of elements. Calling method process() will
     * sort elements between indices iMin and iMax (both inclusive)
     * @param iMin. Index of first element to sort
     * @param iMax. Index of last element to sort
     * @param iD. Recursion depth
     */
    JobSort(int* paElem, int iMin, int iMax, int iD = 1) : thr::AbstractJob()
    {
        m_paElem = paElem;
        m_iMin = iMin;
        m_iMax = iMax;
        m_iMid = -1;
        m_iSpawnCount = 0;
        m_iDepth = iD;
    }

    /**
     * @brief process. Does the actual sorting using Quicksort algorithm
     */
    void process()
    {
        if (m_iMax - m_iMin < QS_LIMIT) {
            insertionSort();
        }   else {
            quickSort();
            // if we are deep enough in the recursion, just use the current
            // job to do the rest of the processing instead of spawning the new
            // ones, in order to prevent QObject allocation overhead
            if (m_iDepth >= 4) {
                int iMid = m_iMid;
                int iMax = m_iMax;

                m_iMax = iMid;
                process();

                m_iMin = iMid + 1;
                m_iMax = iMax;
                process();
            }
        }
        emit signalFinished();
    }

private:
    /**
     * @brief nextSpawnedJob. Returns pointer to the next spawned job
     * @return pointer to the next spawned job
     */
    AbstractJob* nextSpawnedJob()
    {
        if (m_iDepth >= 4)
            return nullptr;

        if (m_bSpawn == false)
            return nullptr;
        ++m_iSpawnCount;

        if (m_iSpawnCount == 1) {
            return new JobSort(m_paElem, m_iMin, m_iMid, m_iDepth + 1);
        }   else if (m_iSpawnCount == 2) {
            return new JobSort(m_paElem, m_iMid + 1, m_iMax, m_iDepth + 1);
        }   else {
            return nullptr;
        }
    }

    /**
     * @brief quickSort. Sorts the elements between indices m_iMin and m_iMax
     * using Quicksort
     */
    void quickSort()
    {
        m_iMid = divide(m_iMin, m_iMax);
        m_bSpawn = true;
    }

    /**
     * @brief insertionSort. Sorts the elements between indices m_iMin and m_iMax
     * using insertion sort
     */
    void insertionSort()
    {
        int iP = m_iMin;
        while (iP < m_iMax) {
            int iMin = iP;
            for (int i = iP + 1; i <= m_iMax; ++i) {
                if (m_paElem[i] < m_paElem[iMin])
                    iMin = i;
            }
            swap(iP++, iMin);
        }
        m_bSpawn = false;
    }

    /**
     * @brief divide. Partially sorts the array around the middle element
     * @param iMin. Minimal index
     * @param iMax. Maximal index
     * @return
     */
    int divide(int iMin, int iMax)
    {
        int iL = iMin;
        int iR = iMax;
        int iPivot = m_paElem[(iL + iR) >> 1];
        while (true) {
            while (m_paElem[iL] < iPivot)
                ++iL;
            while (m_paElem[iR] >= iPivot)
                --iR;

            if (iL >= iR) {
                if (iR < iMin) {
                    swap(iMin, (iMin + iMax) >> 1);
                    iR = iMin;
                }
                return iR;
            }

            swap(iL++, iR--);
        }

        return -1;
    }

private:
    /**
     * @brief swap. Swaps elements on indices iInd1 and iInd2
     * @param iInd1. Index of first element to swap
     * @param iInd2. Index of second element to swap
     */
    void swap(int iInd1, int iInd2)
    {
        int iEl = m_paElem[iInd1];
        m_paElem[iInd1] = m_paElem[iInd2];
        m_paElem[iInd2] = iEl;
    }

    QString desc() const
    {
        QString qs;
        for (int i = m_iMin; i <= m_iMax; ++i) {
            qs += QString::number(m_paElem[i]) + " ";
        }
        return qs;
    }

private:
    /**
     * @brief m_paElem. Pointer to the array of elements to sort
     */
    int* m_paElem;
    /**
     * @brief m_iMin. Index of first element to sort
     */
    int m_iMin;
    /**
     * @brief m_iMax. Index of last element to sort
     */
    int m_iMax;
    /**
     * @brief m_bSpawn. If this flag is true, new jobs will be spawned
     */
    bool m_bSpawn;
    /**
     * @brief m_iMid. Index of the delimiter.
     */
    int m_iMid;
    /**
     * @brief m_iSpawnCount. Number of spawned jobs returned by nextSpawnedJob(),
     */
    int m_iSpawnCount;
    /**
     * @brief m_iDepth. Indicates the depth of the recursion in order to prevent
     * too many jobs to be created
     */
    int m_iDepth;
};

#endif // JOBSORT_H
