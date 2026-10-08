#pragma once

#include <string_view>

// Logging for the FFFCheat library. The library has no log file of its
// own: whoever links it (this repo's ASI, or Rampagio) sets a sink and
// decides where lines go. Lines written before a sink is set are dropped.
// Separate from the ASI's Log.h so a host with its own inline Log::Write
// doesn't get two definitions in one binary.
namespace FFFCheat::Log
{
	using Sink = void (*)(std::string_view line);

	void SetSink(Sink sink);
	void Write(std::string_view line);
}
