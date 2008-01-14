#pragma once
#ifndef FP_THREAD_H
#define FP_THREAD_H

#include "DXUT.h"

#include "fp_global.h"

// Internal structure do not use elsewhere
typedef struct {
    void (*m_JobFunction)(void*);
    void* m_JobData;
    CRITICAL_SECTION m_CurrentlyWorkingCritSect;
    CRITICAL_SECTION m_JobAvailableCritSect;
} fp_Thread_Data;

// It's reccomended to use each single worker thread from one thread only.
class fp_WorkerThread {
public:
    fp_WorkerThread();
    ~fp_WorkerThread();

    // Causes the worker to start working on job and immediately returns. Blocks if
    // thread is already in use.
    void DoWork(void (*JobFunction)(void*), void* JobData);

    // Blocks while thread is working. If other threads are waiting for their jobs to
    // start it is undefined if the function returns before or after these jobs are
    // done.
    void WaitTillWorkDone();
private:
    fp_Thread_Data m_ThreadData;
    HANDLE m_SysThread;    
};

#endif

