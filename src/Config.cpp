#include "Config.h"

#include "Log.h"

#include "..\external\inipp\inipp\inipp.h"

#include <windows.h>

#include <exception>
#include <fstream>
#include <string>

namespace
{
	// Resolves FFFCheat.ini next to this module's .asi rather than trusting
	// the process's working directory. The wide path is opened through
	// MSVC's wide-char stream constructors, so no narrow/wide conversion is
	// needed.
	std::wstring ResolveIniPath()
	{
		HMODULE module = nullptr;
		GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&ResolveIniPath), &module);

		std::wstring path(MAX_PATH, L'\0');
		const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
		path.resize(length);

		const std::size_t slash = path.find_last_of(L"\\/");
		path.resize(slash == std::wstring::npos ? 0 : slash + 1);
		return path + L"FFFCheat.ini";
	}

	// inipp::get_value only writes on success, so seeding with the default
	// gives the fallback.
	bool GetBool(const inipp::Ini<char>::Section& section, const char* key, bool fallback)
	{
		inipp::get_value(section, key, fallback);
		return fallback;
	}

	void LoadImpl(Config::Values& values)
	{
		const std::wstring path = ResolveIniPath();

		inipp::Ini<char> ini;
		{
			std::ifstream in(path);
			if (in)
				ini.parse(in);
		}

		auto& general = ini.sections["General"];
		const Config::Values defaults = values;
		values.AnyButtonCounts = GetBool(general, "AnyButtonCounts", defaults.AnyButtonCounts);
		values.IgnoreEarlyPress = GetBool(general, "IgnoreEarlyPress", defaults.IgnoreEarlyPress);

		// inipp's bool parsing is boolalpha, so write true/false text.
		general["AnyButtonCounts"] = values.AnyButtonCounts ? "true" : "false";
		general["IgnoreEarlyPress"] = values.IgnoreEarlyPress ? "true" : "false";

		std::ofstream out(path, std::ios::trunc);
		if (out)
			ini.generate(out);
		else
			Log::Write("Config: failed to write FFFCheat.ini");
	}
}

namespace Config
{
	Values Load()
	{
		Values values;
		try
		{
			LoadImpl(values);
		}
		catch (const std::exception& e)
		{
			Log::Write(std::string("Config: ") + e.what() + "; using defaults");
			values = Values{};
		}

		Log::Write(std::string("Config: AnyButtonCounts=") + (values.AnyButtonCounts ? "true" : "false") +
			" IgnoreEarlyPress=" + (values.IgnoreEarlyPress ? "true" : "false"));
		return values;
	}
}
