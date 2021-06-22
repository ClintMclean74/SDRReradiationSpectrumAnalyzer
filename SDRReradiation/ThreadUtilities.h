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

#ifndef THREADUTILITIES_H_INCLUDED
#define THREADUTILITIES_H_INCLUDED

#include <iostream>

#ifdef _WIN32
    #include <process.h>
#else
    #include <pthread.h>
#endif

#ifdef STRICT
typedef void *HANDLE;
#if 0 && (_MSC_VER > 1000)
#define DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name
#else
#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name
#endif
#else
typedef void* HANDLE;
#define DECLARE_HANDLE(name) typedef HANDLE name
#endif

typedef HANDLE *PHANDLE;

#ifdef _WIN32
    typedef void ( ThreadFunction)(void *param);
#else
    typedef void* ( ThreadFunction)(void *param);
#endif // _WIN32

namespace ThreadUtilities
{
    //void funct(int a, (void)(*funct2)(int a));
    void funct(int a, ThreadFunction funct2);

    #ifdef _WIN32
        HANDLE CreateThread(ThreadFunction threadFunction, void *param);
    #else
        void* CreateThread(ThreadFunction threadFunction, void *param);
    #endif // _WIN32
    void CloseThread();
}

#endif // THREADUTILITIES_H_INCLUDED
