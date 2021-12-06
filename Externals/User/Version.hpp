#pragma once

#include <compare>
#include <array>
#include <string_view>

namespace ShadowGarden
{
	struct Version
	{
	public:
		enum class Type : unsigned char
		{
			Major,
			Minor,
			Build,
			Revision
		};
		using data_type = unsigned short;

		constexpr Version() = default;

		constexpr Version(data_type maj, data_type min, data_type build = 0, data_type rev = 0) noexcept :
			m_Major(maj), m_Minor(min), m_Build(build), m_Revision(rev) {  }

		constexpr Version(std::array<data_type, 4> arr) noexcept :
			m_Major(arr[0]), m_Minor(arr[1]), m_Build(arr[2]), m_Revision(arr[3]) { }

		constexpr Version(std::string_view str) noexcept :
			Version(from_string(str)) {  }

		constexpr data_type operator[](Type ty) const noexcept
		{
			switch (ty)
			{
			case Type::Major:	return m_Major;
			case Type::Minor:	return m_Minor;
			case Type::Build:	return m_Build;
			case Type::Revision:
			default:			return m_Revision;
			}
		}

		constexpr data_type& operator[](Type ty) noexcept
		{
			switch (ty)
			{
			case Type::Major:	return m_Major;
			case Type::Minor:	return m_Minor;
			case Type::Build:	return m_Build;
			case Type::Revision:
			default:			return m_Revision;
			}
		}

		data_type& major() noexcept					{ return m_Major; }
		constexpr data_type major() const noexcept	{ return m_Major; }

		data_type& minor() noexcept					{ return m_Minor; }
		constexpr data_type minor() const noexcept	{ return m_Minor; }

		data_type& build() noexcept					{ return m_Build; }
		constexpr data_type build() const noexcept	{ return m_Build; }

		data_type& revision() noexcept				{ return m_Revision; }
		constexpr data_type revision() const noexcept{ return m_Revision; }

		constexpr auto to_array() const noexcept { return std::array{ major(), minor(), build(), revision() }; }

		static constexpr Version from_string(std::string_view str)
		{
			int iter = 0;
			Type type = Type::Major;
			Version ver;
			
			for (const char c : str)
			{
				if (type > Type::Revision)
					break;

				if (c == '.')
				{
					iter = 0;
					type = static_cast<Type>(static_cast<int>(type) + 1);
					continue;
				}

				iter *= 10;
				iter += c - '0';

				ver[type] = iter;
			}

			return ver;
		}

		constexpr auto operator<=>(const Version&) const noexcept = default;

		static constexpr Version Null() { return { }; }

	private:
		data_type
			m_Major = 0,
			m_Minor = 0,
			m_Build = 0,
			m_Revision = 0;
	};
}