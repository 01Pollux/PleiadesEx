#pragma once

#include <memory>
#include <vector>
#include "SGDefines.hpp"


SG_NAMESPACE_BEGIN;

class DataAndSize
{
public:
	using data_type = std::byte;
	using data_ptr = std::byte*;
	using data_arr = std::byte[];

	DataAndSize(const size_t size) :
		m_Size(size),
		m_Data(size != 0 ? std::make_unique<data_arr>(size) : nullptr)
	{ }

	bool is_void() const noexcept
	{
		return !m_Size;
	}

	data_ptr data() noexcept
	{
		return m_Data.get();
	}

	const data_ptr data() const noexcept
	{
		return m_Data.get();
	}

	size_t size() const noexcept
	{
		return m_Size;
	}

private:
	std::unique_ptr<data_arr>
		m_Data;

	size_t	m_Size;
};


class PassArgs
{
	friend class CallContext;
public:
	using data_type = DataAndSize::data_type;
	using data_ptr = DataAndSize::data_ptr;
	using data_arr = DataAndSize::data_arr;

	static constexpr size_t ThisPointer = 0u;

	PassArgs(const std::vector<std::pair<size_t, bool>>& args, bool this_ptr) :
		m_ContainsThis(this_ptr),
		m_ArgInfo(args.size() ? std::make_unique<ArgInfo[]>(args.size()) : nullptr)
	{
		if (m_ArgInfo)
		{
			m_CurData.reserve(args.size());
			m_NewData.reserve(args.size());

			size_t offs{ };
			for (const auto& arg : args)
			{
				m_CurData.emplace_back(arg.first);
				m_NewData.emplace_back(arg.first);
				m_ArgInfo[offs++].IsConst = arg.second;
			}
		}
	}

	const data_ptr get_ptr(size_t index, size_t offset = 0) const noexcept;

	template<typename _Ty>
	const _Ty* get_ptr(size_t index, size_t offset = 0) const noexcept
	{
		return std::bit_cast<const _Ty*>(get_ptr(index, offset));
	}

	template<typename _Ty>
	const _Ty& get(size_t index, size_t offset = 0) const noexcept
	{
		return *std::bit_cast<const _Ty*>(get_ptr(index, offset));
	}

	template<typename _Ty = void>
	const _Ty* get_this() const noexcept
	{
		return m_ContainsThis ? *std::bit_cast<const _Ty**>(m_CurData[0].data()) : nullptr;
	}

	template<typename _Ty = void>
	_Ty* get_this() noexcept
	{
		return m_ContainsThis ? *std::bit_cast<_Ty**>(m_CurData[ThisPointer].data()) : nullptr;
	}

	const data_ptr get_new_ptr(size_t index, size_t offset = 0) const noexcept;

	template<typename _Ty>
	const _Ty* get_new_ptr(size_t index, size_t offset = 0) const noexcept
	{
		return std::bit_cast<const _Ty*>(get_new_ptr(index, offset));
	}

	template<typename _Ty>
	const _Ty& get_new(size_t index, size_t offset = 0) const noexcept
	{
		return *std::bit_cast<const _Ty*>(get_new_ptr(index, offset));
	}


	void set_ptr(size_t index, const void* data, size_t offset = 0, int size = -1) noexcept;

	template<typename _Ty>
	void set(size_t index, const _Ty& val, size_t offset = 0) noexcept
	{
		set_ptr(index, &val, offset, sizeof(_Ty));
	}

	void reset(size_t index, size_t offset = 0);
	void reset();

	size_t size() const noexcept { return m_CurData.size(); }
	size_t arg_size(size_t i) const noexcept { return m_CurData[i + (m_ContainsThis ? 1 : 0)].size(); }

	bool has_this_ptr() const noexcept { return m_ContainsThis; }
	bool empty() const noexcept { return size() == (has_this_ptr() ? 1 : 0); }

	size_t is_const(size_t pos) const noexcept { return m_ArgInfo[pos].IsConst; }
	size_t has_changed(size_t pos) const noexcept { return m_ArgInfo[pos].Changed; }

protected:
	struct ArgInfo
	{
		bool Changed{ };
		bool IsConst{ };
	};

	std::vector<DataAndSize> m_CurData;
	std::vector<DataAndSize> m_NewData;

	std::unique_ptr<ArgInfo[]> m_ArgInfo;

	bool	m_ContainsThis;
};


class PassRet
{
	friend class CallContext;
public:
	using data_type = DataAndSize::data_type;
	using data_ptr = DataAndSize::data_ptr;
	using data_arr = DataAndSize::data_arr;

	PassRet(const size_t& size) :
		m_CurData(size),
		m_NewData(size)
	{ }

	const data_ptr get_ptr(size_t offset = 0) const noexcept;

	template<typename _Ty>
	const _Ty* get_ptr(size_t offset = 0) const noexcept
	{
		return std::bit_cast<const _Ty*>(get_ptr(offset));
	}

	template<typename _Ty>
	const _Ty& get(size_t offset = 0) const noexcept
	{
		return *std::bit_cast<const _Ty*>(get_ptr(offset));
	}

	const data_ptr get_new_ptr(size_t offset = 0) const noexcept;

	template<typename _Ty>
	const _Ty* get_new_ptr(size_t offset = 0) const noexcept
	{
		return std::bit_cast<const _Ty*>(get_new_ptr(offset));
	}

	template<typename _Ty>
	const _Ty& get_new(size_t offset = 0) const noexcept
	{
		return *std::bit_cast<const _Ty*>(get_new_ptr(offset));
	}


	void set_ptr(const void* data, size_t offset = 0, int size = -1) noexcept;

	template<typename _Ty>
	void set(const _Ty& val, size_t offset = 0) noexcept
	{
		set_ptr(std::bit_cast<const void*>(&val), offset, sizeof(_Ty));
	}

	void reset() noexcept;

	size_t size() const noexcept { return m_CurData.size(); }

	bool is_void() const noexcept
	{
		return m_NewData.is_void();
	}

	bool has_changed() const noexcept
	{
		return m_Changed;
	}

protected:
	DataAndSize	m_CurData;
	DataAndSize	m_NewData;
	bool	m_Changed{ };
};


inline const PassArgs::data_ptr PassArgs::get_ptr(size_t index, size_t offset) const noexcept
{
	index += m_ContainsThis ? 1 : 0;
	return m_CurData[index].data() + offset;
}

inline const PassArgs::data_ptr PassArgs::get_new_ptr(size_t index, size_t offset) const noexcept
{
	index += m_ContainsThis ? 1 : 0;
	return m_NewData[index].data() + offset;
}

inline void PassArgs::set_ptr(size_t index, const void* data, size_t offset, int size) noexcept
{
	index += m_ContainsThis ? 1 : 0;
	if (!is_const(index))
	{
		memcpy(m_NewData[index].data() + offset, data, size == -1 ? m_NewData[index].size() : size);
		m_ArgInfo[index].Changed = true;
	}
}

inline void PassArgs::reset(size_t index, size_t offset)
{
	index += m_ContainsThis ? 1 : 0;
	memcpy(m_NewData[index].data() + offset, m_CurData[index].data() + offset, m_NewData[index].size());
	m_ArgInfo[index].Changed = false;
}

inline void PassArgs::reset()
{
	for (size_t index = m_ContainsThis ? 1 : 0; index < size(); index++)
	{
		std::copy_n(m_CurData[index].data(), m_NewData[index].size(), m_NewData[index].data());
		m_ArgInfo[index].Changed = false;
	}
}


inline const PassRet::data_ptr PassRet::get_ptr(size_t offset) const noexcept
{
	return m_CurData.data() + offset;
}

inline const PassRet::data_ptr PassRet::get_new_ptr(size_t offset) const noexcept
{
	return m_NewData.data() + offset;
}

inline void PassRet::set_ptr(const void* data, size_t offset, int size) noexcept
{
	memcpy(m_NewData.data() + offset, data, size == -1 ? m_NewData.size() : size);
	m_Changed = true;
}

inline void PassRet::reset() noexcept
{
	memcpy(m_NewData.data(), m_CurData.data(), m_NewData.size());
	m_Changed = false;
}

SG_NAMESPACE_END;