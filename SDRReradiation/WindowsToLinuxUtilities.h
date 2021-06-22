/*
 * SDRReradiation - Code system to detect biologically reradiated
 * electromagnetic energy from humans
 * Copyright (C) 2021 by Clint Mclean <clint@getfitnowapps.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WINTYPES_H_INCLUDED
#define WINTYPES_H_INCLUDED

#ifdef _WIN32
    #include <winsock.h>
    #include "GL/glew.h"
    #include "GL/wglew.h"
    #include "DebuggingUtilities.h"

    long GetTime();

    void WriteDataToSocket(SOCKET sd, char* dataStr, int dataLength);

    void CloseSocket(int sd);

    void UnitializeSockets();

    HANDLE CreateSemaphoreObject(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName);

    int WaitForSemaphore(HANDLE* semaphore, LONG duration);

    int WaitForMultipleSemaphores(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);

    bool ReleaseSemaphoreObject(HANDLE* semaphore, LONG releaseCount, LPLONG prevReleaseCount);

    void Timeout(DWORD dwMilliseconds);

    bool WGLExtensionSupported(const char *extension_name);
#else
    #include <unistd.h>
    #include <time.h>
    #include <semaphore.h>
    #include <string>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <semaphore.h>
    #include <GL/glew.h>

    #include "DebuggingUtilities.h"

    #define WAIT_OBJECT_0 0

    #define WAIT_TIMEOUT 0x00000102L

    typedef long DWORD;

    typedef int SOCKET;

    //struct sockaddr_in server;				/* Information about the server */

    long GetTime();

    void WriteDataToSocket(SOCKET sd, char* dataStr, int dataLength);

    void CloseSocket(int sd);

    void UnitializeSockets();

    sem_t CreateSemaphoreObject(int *lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, std::string lpName);

    int WaitForSemaphore(sem_t* semaphore, long duration);

    int WaitForMultipleSemaphores(DWORD nCount, sem_t** semaphores, bool bWaitAll, DWORD dwMilliseconds);

    bool ReleaseSemaphoreObject(sem_t* semaphore, long releaseCount, long* prevReleaseCount);

    void Timeout(DWORD dwMilliseconds);


#endif // _WIN32

#endif // WINTYPES_H_INCLUDED
