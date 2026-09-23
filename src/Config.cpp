#include "Config.h"

#include "Log.h"
#include "LogFallback.h"

#include "..\external\inipp\inipp\inipp.h"

#include <windows.h>

#include <exception>
#include <fstream>
#include <string>

namespace
{
	// Where FFFCheat.ini is loaded from and saved to: next to the .asi, or
	// %LOCALAPPDATA%\RDR2ASIMods\FFFCheat.ini when the game folder isn't
	// writable -- starting from the game folder's copy if there is one (see
	// LogFallback::ResolveSettings). Resolved once per session.
	const LogFallback::SettingsPaths& IniPaths()
	{
		static const LogFallback::SettingsPaths paths = LogFallback::ResolveSettings(
			LogFallback::ModuleDirectory(), L"FFFCheat.ini", LogFallback::FallbackDirectory());
		return paths;
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
		const LogFallback::SettingsPaths& paths = IniPaths();

		inipp::Ini<char> ini;
		{
			std::ifstream in(paths.read);
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

		if (paths.usedFallback)
			Log::Write("Config: the game folder isn't writable, so settings are saved to " +
				LogFallback::ToUtf8(paths.write));

		std::ofstream out(paths.write, std::ios::trunc);
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
