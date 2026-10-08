#include "FFFCheatLog.h"

#include <atomic>

namespace FFFCheat::Log
{
	namespace
	{
		std::atomic<Sink> g_sink{ nullptr };
	}

	void SetSink(Sink sink)
	{
		g_sink.store(sink);
	}

	void Write(std::string_view line)
	{
		if (const Sink sink = g_sink.load())
			sink(line);
	}
}
