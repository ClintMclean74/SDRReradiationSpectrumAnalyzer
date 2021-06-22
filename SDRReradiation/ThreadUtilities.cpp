#pragma once
#include "ThreadUtilities.h"

namespace ThreadUtilities
{
    #ifdef _WIN32
        HANDLE CreateThread(ThreadFunction threadFunction, void *param)
        {
            HANDLE threadHandle = (HANDLE)_beginthread(threadFunction, 0, param);
            //bool result = SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);

            return threadHandle;
        }

        void CloseThread()
        {
            _endthread();
        }
    #else
        void* CreateThread(ThreadFunction threadFunction, void *param)
        {
            pthread_t *newThread = new pthread_t();
            int result = pthread_create(newThread, NULL, threadFunction, param);

            return newThread;
        }

        void CloseThread()
        {
            pthread_exit(NULL);
        }
    #endif
}
