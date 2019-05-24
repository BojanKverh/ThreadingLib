#ifndef IMAGEJOB_H
#define IMAGEJOB_H

#include <QImage>
#include <QDebug>

#include "abstractjob.h"

/**
 * @brief The ImageJob class. This class is used to do edge detection on
 * a part of an image
 */
class ImageJob : public thr::AbstractJob
{
    Q_OBJECT

public:
    /**
     * @brief ImageJob. Constructor
     * @param rInImg. Read only reference to the input image
     * @param rOutImg. Reference to the output image
     * @param iRowStart. Row index, where processing will start (included)
     * @param iRowEnd. Row index, where processing will finish (excluded)
     * @param iColStart. Column index, where processing will start (included)
     * @param iColEnd. Column index, where processing will finish (excluded)
     */
    ImageJob(
            const QImage& rInImg,
            QImage& rOutImg,
            int iRowStart,
            int iRowEnd,
            int iColStart,
            int iColEnd
            ) :
        thr::AbstractJob(),
        m_rInImg(rInImg),
        m_rOutImg(rOutImg)
    {
        m_iRowStart = iRowStart;
        m_iRowEnd = iRowEnd;
        m_iColStart = iColStart;
        m_iColEnd = iColEnd;
    }

    /**
     * @brief process. Does the actual image processing
     */
    void process()
    {
        int iSum;
        int iCount;
        for (int iR = m_iRowStart; iR < m_iRowEnd; ++iR) {
            for (int iC = m_iColStart; iC < m_iColEnd; ++iC) {
                check(iR, iC, iSum, iCount);
                if (iCount > 0) {
                    int iP = 16*iSum / iCount;
                    if (iP > 255)
                        iP = 255;
                    m_rOutImg.setPixel(iC, iR, qRgb(iP, iP, iP));
                }
            }
        }

        qDebug() << "Process finished" << m_iRowStart
                 << m_iRowEnd << m_iColStart << m_iColEnd;
        emit signalFinished();
    }

private:
    /**
     * @brief check. Calculates the summed difference for given pixel
     * @param iR. Pixel row
     * @param iC. Pixel column
     * @param riSum. Summed difference of pixel values will be stored into this variable
     * @param riCount. Summed weights will be stored into this variable
     */
    void check(int iR, int iC, int& riSum, int& riCount) const
    {
        int iRMin = qMax(0, iR - 1);
        int iRMax = qMin(m_rInImg.height() - 1, iR + 1);
        int iCMin = qMax(0, iC - 1);
        int iCMax = qMin(m_rInImg.width() - 1, iC + 1);

        QRgb rgb = m_rInImg.pixel(iC, iR);
        int iF;
        riSum = 0;
        riCount = 0;

        for (int iRow = iRMin; iRow <= iRMax; ++iRow) {
            for (int iCol = iCMin; iCol <= iCMax; ++iCol) {

                if ((iRow != iR) || (iCol != iC)) {
                    if ((iRow == iR) || (iCol == iC))
                        iF = 6;
                    else
                        iF = 3;

                    QRgb pix = m_rInImg.pixel(iCol, iRow);
                    riSum +=
                            abs(qRed(pix) - qRed(rgb)) +
                            abs(qGreen(pix) - qGreen(rgb)) +
                            abs(qBlue(pix) - qBlue(rgb));
                    riCount += iF;
                }
            }
        }
    }

private:
    /**
     * @brief m_rInImg. Read only reference to the input image
     */
    const QImage& m_rInImg;
    /**
     * @brief m_rOutImg. Reference to the output image
     */
    QImage& m_rOutImg;
    /**
     * @brief m_iRowStart. Row index where processing will start (included)
     */
    int m_iRowStart;
    /**
     * @brief m_iRowEnd. Row index, where processing will finish (excluded)
     */
    int m_iRowEnd;
    /**
     * @brief m_iColStart. Column index, where processing will start (included)
     */
    int m_iColStart;
    /**
     * @brief m_iColEnd. Column index, where processing will finish (excluded)
     */
    int m_iColEnd;
};

#endif // IMAGEJOB_H
