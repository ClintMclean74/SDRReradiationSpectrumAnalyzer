#pragma once

namespace DebuggingUtilities
{
	static const bool DEBUGGING = false;

	static const bool RECEIVE_TEST_DATA = false;
	static const bool TEST_CORRELATION = false;

	void DebugPrint(const char * format, ...);
}