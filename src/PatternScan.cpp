#include "PatternScan.h"

#include <windows.h>

#include <cstring>
#include <string>
#include <vector>

namespace
{
	struct ParsedPattern
	{
		std::vector<std::uint8_t> bytes;
		std::vector<bool> mask;
	};

	ParsedPattern Parse(std::string_view pattern)
	{
		ParsedPattern parsed;

		std::size_t i = 0;
		while (i < pattern.size())
		{
			while (i < pattern.size() && pattern[i] == ' ')
				++i;
			if (i >= pattern.size())
				break;

			if (pattern[i] == '?')
			{
				parsed.bytes.push_back(0);
				parsed.mask.push_back(false);
				++i;
				if (i < pattern.size() && pattern[i] == '?')
					++i;
			}
			else
			{
				parsed.bytes.push_back(static_cast<std::uint8_t>(
					std::stoul(std::string(pattern.substr(i, 2)), nullptr, 16)));
				parsed.mask.push_back(true);
				i += 2;
			}
		}

		return parsed;
	}
}

namespace PatternScan
{
	std::optional<std::uintptr_t> FindInMainModule(std::string_view pattern)
	{
		auto base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
		if (!base)
			return std::nullopt;

		auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
		auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
		const std::size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;
		const ParsedPattern parsed = Parse(pattern);

		if (parsed.bytes.empty() || imageSize < parsed.bytes.size())
			return std::nullopt;

		std::size_t firstConcrete = 0;
		while (firstConcrete < parsed.bytes.size() && !parsed.mask[firstConcrete])
			++firstConcrete;

		if (firstConcrete == parsed.bytes.size())
			return std::nullopt;

		auto* searchStart = base + firstConcrete;
		std::size_t remaining = imageSize - firstConcrete;
		for (;;)
		{
			if (remaining < parsed.bytes.size() - firstConcrete)
				break;

			const std::size_t searchable = remaining - (parsed.bytes.size() - firstConcrete - 1);
			void* found = std::memchr(searchStart, parsed.bytes[firstConcrete], searchable);
			if (!found)
				break;

			auto* candidateAnchor = static_cast<std::uint8_t*>(found);
			auto* candidateStart = candidateAnchor - firstConcrete;
			bool matched = true;
			for (std::size_t i = 0; i < parsed.bytes.size(); ++i)
			{
				if (parsed.mask[i] && candidateStart[i] != parsed.bytes[i])
				{
					matched = false;
					break;
				}
			}

			if (matched)
				return reinterpret_cast<std::uintptr_t>(candidateStart);

			const std::size_t advanced = static_cast<std::size_t>(candidateAnchor - searchStart) + 1;
			searchStart += advanced;
			remaining -= advanced;
		}

		return std::nullopt;
	}

	std::uintptr_t ResolveRip(std::uintptr_t instruction, std::ptrdiff_t operandOffset)
	{
		const auto operand = instruction + operandOffset;
		const auto displacement = *reinterpret_cast<const std::int32_t*>(operand);
		return operand + sizeof(displacement) + displacement;
	}
}
