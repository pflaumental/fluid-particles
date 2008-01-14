#include "DXUT.h"
#include "fp_thread.h"

DWORD WINAPI fp_Thread_ThreadFunc(LPVOID Data) {
    fp_Thread_Data* threadData = (fp_Thread_Data*) Data;
    while(true) {
        // Wait till there's a job to be done
        EnterCriticalSection(&threadData->m_JobAvailableCritSect);
        // Do the job
        threadData->m_JobFunction(threadData->m_JobData);
        // Signal that work on job is done
        LeaveCriticalSection(&threadData->m_CurrentlyWorkingCritSect);
    }
    return 0;
}

fp_WorkerThread::fp_WorkerThread() {
    InitializeCriticalSection(&m_ThreadData.m_CurrentlyWorkingCritSect);
    InitializeCriticalSection(&m_ThreadData.m_JobAvailableCritSect);

    // No job available at beginning
    EnterCriticalSection(&m_ThreadData.m_JobAvailableCritSect);

    m_SysThread = CreateThread(NULL, 0, fp_Thread_ThreadFunc, &m_ThreadData, NULL, 0);
    ResumeThread(m_SysThread);
}

fp_WorkerThread::~fp_WorkerThread() {
    TerminateThread(m_SysThread,0);
    DeleteCriticalSection(&m_ThreadData.m_CurrentlyWorkingCritSect);
    DeleteCriticalSection(&m_ThreadData.m_JobAvailableCritSect);
}

void fp_WorkerThread::DoWork(void (JobFunction)(void*), void* JobData) {
    // Wait until worker thread is available
    EnterCriticalSection(&m_ThreadData.m_CurrentlyWorkingCritSect);

    // Assign job
    m_ThreadData.m_JobData = JobData;
    m_ThreadData.m_JobFunction = JobFunction;

    // Signal the worker thread that there's a job to be done
    LeaveCriticalSection(&m_ThreadData.m_JobAvailableCritSect);
}

void fp_WorkerThread::WaitTillWorkDone() {
    EnterCriticalSection(&m_ThreadData.m_CurrentlyWorkingCritSect);
    LeaveCriticalSection(&m_ThreadData.m_CurrentlyWorkingCritSect);
}
