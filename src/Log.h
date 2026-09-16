#pragma once

#include <windows.h>

#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace Log
{
	inline void Write(const std::string& message)
	{
		SYSTEMTIME time{};
		GetLocalTime(&time);

		std::ostringstream formatted;
		formatted << '['
			<< std::setfill('0') << std::setw(2) << time.wHour << ':'
			<< std::setfill('0') << std::setw(2) << time.wMinute << ':'
			<< std::setfill('0') << std::setw(2) << time.wSecond << '.'
			<< std::setfill('0') << std::setw(3) << time.wMilliseconds
			<< "] " << message;

		const std::string line = formatted.str() + "\n";
		static std::mutex mutex;
		std::lock_guard lock(mutex);
		std::ofstream file("FFFCheat.log", std::ios::app);
		if (file)
			file << line;
		OutputDebugStringA(line.c_str());
	}
}
