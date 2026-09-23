#pragma once

#include "LogFallback.h"

#include <windows.h>

#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

// Appends to FFFCheat.log next to FFFCheat.asi, or to
// %LOCALAPPDATA%\RDR2ASIMods\FFFCheat.log when the game folder isn't
// writable (see LogFallback.h). Never throws.
namespace Log
{
	namespace detail
	{
		struct Target
		{
			std::wstring path; // empty: nothing writable, file output is skipped
			std::string redirectNotice;
		};

		inline Target ResolveTarget(const std::wstring& preferredDir, const std::wstring& fallbackDir)
		{
			Target target;
			const LogFallback::Resolved resolved = LogFallback::Resolve(preferredDir, L"FFFCheat.log", fallbackDir);
			target.path = resolved.path;
			if (resolved.usedFallback && !resolved.path.empty())
				target.redirectNotice = "Log redirected here: could not write " + LogFallback::ToUtf8(resolved.rejectedPath);
			return target;
		}
	}

	inline void Write(const std::string& message)
	{
		try
		{
			SYSTEMTIME time{};
			GetLocalTime(&time);

			std::ostringstream formatted;
			formatted << '['
				<< std::setfill('0') << std::setw(2) << time.wHour << ':'
				<< std::setfill('0') << std::setw(2) << time.wMinute << ':'
				<< std::setfill('0') << std::setw(2) << time.wSecond << '.'
				<< std::setfill('0') << std::setw(3) << time.wMilliseconds
				<< "] ";
			const std::string prefix = formatted.str();

			static std::mutex mutex;
			std::lock_guard lock(mutex);
			static detail::Target target = detail::ResolveTarget(LogFallback::ModuleDirectory(), LogFallback::FallbackDirectory());

			const std::string line = prefix + message + "\n";
			if (!target.path.empty())
			{
				std::ofstream file(target.path, std::ios::app);
				if (file)
				{
					if (!target.redirectNotice.empty())
					{
						file << prefix << target.redirectNotice << "\n";
						target.redirectNotice.clear();
					}
					file << line;
				}
			}
			OutputDebugStringA(line.c_str());
		}
		catch (...)
		{
		}
	}
}
