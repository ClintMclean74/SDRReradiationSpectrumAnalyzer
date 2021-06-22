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

#ifndef GNURADIOUTILITIES
#define GNURADIOUTILITIES

#include <string>
#include "pstdint.h"

//typedef size_t (* XMLRPCFunction)(void *ptr, size_t size, size_t nmemb, void *);
typedef void (* XMLRPCFunction)(char *dataStr);

class GNU_Radio_Utilities
{
        public:
		static const char* GNU_RADIO_XMLRPC_SERVER_ADDRESS;
		static const uint32_t GNU_RADIO_XMLRPC_SERVER_ADDRESS_PORT;
		static const uint32_t GNU_RADIO_DATA_STREAMING_ADDRESS_PORT;

        std::string CallXMLRPC(const char* data);
};

#endif // GNURADIOUTILITIES
