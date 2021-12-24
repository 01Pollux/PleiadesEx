#pragma once

#include "Version.hpp"
#include <format>
#include <nlohmann/Json.hpp>

namespace SG = ShadowGarden;

namespace std
{
	template<class _CharT>
	struct formatter<SG::Version, _CharT>
	{
		template<typename _FmtCtx>
		auto parse(_FmtCtx& ctx)
		{
			return ctx.end();
		}

		template<typename _FmtCtx>
		auto format(const SG::Version& ver, _FmtCtx& fc)
		{
			return std::format_to(
				fc.out(),
				"{0}.{1}.{2}.{3}", 
				ver.major(), 
				ver.minor(),
				ver.build(),
				ver.revision()
			);
		}
	};

	template <class _CharT, class _Traits>
	basic_ostream<_CharT, _Traits>& operator<<(basic_ostream<_CharT, _Traits>& os, const SG::Version& ver)
	{
		return os << ver.major() << '.' << ver.minor() << '.' << ver.build() << '.' << ver.revision();
	}
}

namespace nlohmann
{
	inline void to_json(json& js, const SG::Version& version)
	{
		js = std::format("{}", version);
	}

	inline void from_json(const json& js, SG::Version& version)
	{
		version = SG::Version::from_string(js);
	}
}