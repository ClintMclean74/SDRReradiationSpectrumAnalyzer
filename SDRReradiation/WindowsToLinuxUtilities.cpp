#include "WindowsToLinuxUtilities.h"

#ifdef _WIN32
    long GetTime()
    {
        return GetTickCount();
    }

    void WriteDataToSocket(SOCKET sd, char* dataStr, int dataLength)
    {
        send(sd, dataStr, dataLength, 0);
    }

    void CloseSocket(int sd)
    {
        closesocket(sd);
    }

    void UnitializeSockets()
    {
        WSACleanup();
    }

    HANDLE CreateSemaphoreObject(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName)
    {
        HANDLE semaphore = CreateSemaphore(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);

        if (semaphore == NULL)
        {
            if (DebuggingUtilities::DEBUGGING)
			{
                DebuggingUtilities::DebugPrint("CreateSemaphoreObject error: %d\n", GetLastError());
			}
        }

        return semaphore;
    }

    int WaitForSemaphore(HANDLE* semaphore, LONG duration)
    {
       return WaitForSingleObject(*semaphore, duration);
    }

    int WaitForMultipleSemaphores(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
    {
       return WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
    }

    bool ReleaseSemaphoreObject(HANDLE* semaphore, LONG releaseCount, LPLONG prevReleaseCount)
    {
        if (ReleaseSemaphore(*semaphore, 1, NULL)==0)
        {
            if (DebuggingUtilities::DEBUGGING)
                DebuggingUtilities::DebugPrint("ReleaseSemaphoreObject error: %d\n", GetLastError());

            return false;
        }

        return true;
    }

    void Timeout(DWORD dwMilliseconds)
    {
        Sleep(dwMilliseconds);
    }

    bool GLExtensionSupported(const char *extension_name)
    {
        // this is pointer to function which returns pointer to string with list of all wgl extensions
        PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

        // determine pointer to wglGetExtensionsStringEXT function
        _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");

        if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
        {
            // string was not found
            return false;
        }

        // extension is supported
        return true;
    }

#else
    struct sockaddr_in server2;				/* Information about the server */

    long GetTime()
    {
        struct timespec ts;
        unsigned theTick = 0U;
        clock_gettime( CLOCK_REALTIME, &ts );
        theTick  = ts.tv_nsec / 1000000;
        theTick += ts.tv_sec * 1000;
        return theTick;
    }

    void WriteDataToSocket(SOCKET sd, char* dataStr, int dataLength)
    {
        write(sd, dataStr, dataLength);
    }

    void CloseSocket(int sd)
    {
        close(sd);
    }

    void UnitializeSockets()
    {
    }

    sem_t CreateSemaphoreObject(int *lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, std::string lpName)
    {
        sem_t *semaphore = new sem_t();

        int result = sem_init(semaphore, 0, lMaximumCount);

        if (result != 0)
        {
            if (DebuggingUtilities::DEBUGGING)
			{
                DebuggingUtilities::DebugPrint("CreateSemaphoreObject error: %d\n", result);
			}
        }

        return *semaphore;
    }

    int WaitForSemaphore(sem_t* semaphore, long duration)
    {
        struct timespec ts;

        ts.tv_sec = duration/1000;

        //return sem_timedwait(semaphore, &ts);

        return sem_wait(semaphore);
    }

    int WaitForMultipleSemaphores(DWORD nCount, sem_t** semaphores, bool bWaitAll, DWORD dwMilliseconds)
    {
        for (int i=0; i<nCount; i++)
        {
            //sem_wait((sem_t *) &semaphores[i]);
            sem_wait(semaphores[i]);
        }

       return 0;
    }


    bool ReleaseSemaphoreObject(sem_t* semaphore, long releaseCount, long* prevReleaseCount)
    {
        int result = sem_post(semaphore);

        if (result == -1)
            return false;

        return true;
    }

    void Timeout(DWORD milliSeconds)
    {
        sleep(milliSeconds/1000);
    }

    bool GLExtensionSupported(const char *extension_name)
    {

        if (!glewIsSupported(extension_name))
        {    // string was not found
            return false;
        }

        // extension is supported
        return true;
    }
#endif // _WIN32
