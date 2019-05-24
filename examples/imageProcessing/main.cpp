#include <QCoreApplication>
#include <QTime>
#include <QDebug>

#include "jobmanager.h"
#include "imagejob.h"

#define PARTS               8

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QImage im(":/images/Panorama.jpg");
    QImage imOut(im.width(), im.height(), QImage::Format_ARGB32);
    imOut.fill(Qt::black);
    ImageJob* pJob;

    // let's first try with 8 threads
    thr::JobManager jm(8);
    int iRMin = 0;
    int iRDiff = im.height()/PARTS;
    int iCDiff = im.width()/PARTS;

    for (int iR = 0; iR < PARTS; ++iR) {
        int iCMin = 0;
        for (int iC = 0; iC < PARTS; ++iC) {
            pJob = new ImageJob(
                        im,
                        imOut,
                        iRMin,
                        iR < PARTS-1? iRMin + iRDiff : im.height(),
                        iCMin,
                        iC < PARTS-1? iCMin + iCDiff : im.width()
                                );
            jm.appendJob(pJob);
            iCMin += iCDiff;
        }
        iRMin += iRDiff;
    }
    QTime tm;
    tm.start();
    jm.start();
    while (jm.isRunning() == true) {
        a.processEvents();
    }
    qDebug() << "Image processing in 8 thread took" << tm.elapsed() << "[ms]";
    imOut.save("output.png");

    thr::JobManager jm2(1);
    pJob = new ImageJob(im, imOut, 0, im.height(), 0, im.width());
    jm2.appendJob(pJob);
    tm.start();
    jm2.start();
    while (jm2.isRunning() == true) {
        a.processEvents();
    }
    //pJob->process();
    qDebug() << "Image processing in 1 thread took" << tm.elapsed() << "[ms]";
    imOut.save("output2.png");

    return a.exec();
}
