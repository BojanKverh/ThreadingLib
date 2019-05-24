# ThreadingLib
Split the task into several smaller tasks and use ThreadingLib to use all your processor cores to complete the task in shorter time

This library is intended to maximize the use of your computer processor(s).
It is meant to be used in cases, where there is a huge computing task to be done,
which can be divided into several smaller tasks that can be processed in parallel.
This library takes away the management of different threads and waiting for their
finish before assigning them the next job to process.

The processing of the smaller tasks should be implemented in the process method of
the class derived from abstract class AbstractJob.

In this library there are two
classes, which handle the processing of number of smaller tasks:
- class <b>JobManager</b>: if all the smaller tasks can be created at once, this is the
  preferred class to use. Create every small task needed to complete the
  computing and add it to JobManager via
  appendJob() method. Then call JobManager::start() method and JobManager will take
  care of the rest. When all the tasks are processed, JobManager will emit
  signal signalFinished.
- class <b>AbstractSessionManager</b>: if creating all the smaller tasks at once exceeds
  your system memory or some other system resource, you have to divide tasks into 
  sessions. AbstractSessionManager is an abstract class, which has to be derived and its
  methods sessionCount() and initNextSession() have to be implemented. Method
  sessionCount should return the total number of sessions the tasks are divided into.
  Method initNextSession should create each small task belonging to the current
  session and add them into the internal JobManager object. Calling
  AbstractSessionManager::start() method will initialize first session and process
  all the tasks in it, then it will delete the tasks from first session,
  initialize second session and process all the tasks in it and so on until all
  the sessions are completed or the processing is stopped by calling the stop()
  method or too many errors occured during one session.

The library comes with a few examples of usage (<i>examples/qsort</i> and <i>examples/imageProcessing</i>), unit tests (<i>tests/UnitTests</i>) and extensive class documentation (<i>doc/html</i>).

<h2>Compiling</h2>
This library is based on Qt's multithreading capabilities, so it should be crossplatform.

To compile it, just execute:


<code>make all</code>


in the command line in the root directory. The library will be compiled in the <i>src</i> directory. For successful build, Qt5 libraries are needed.

You can also navigate to <i>src</i> directory and execute:

<code>
  qmake
  
  make
</code>


<h2>Finding it useful?</h2>

[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=CY962QPSSHPHY)

