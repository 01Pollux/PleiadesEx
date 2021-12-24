#pragma once

#include <unordered_map>
#include <asmjit/asmjit.h>
#include <nlohmann/Json.hpp>

#include "TypeTable.hpp"

SG_NAMESPACE_BEGIN;

class CallContext;
namespace DetourDetail
{
	class TypeInfo
	{
		friend class SigBuilder;
		friend class CallContext;
	public:
		enum class RetType : char8_t
		{
			Void,
			RetReg,
			RetRegx2,
			RetMem
		};

		struct RegAndSize
		{
			asmjit::BaseReg Reg;
			size_t		Size{ };
			asmjit::BaseReg ExtraReg;
		};

		bool has_ret() const noexcept
		{
			return m_RetType != RetType::Void;
		}

		bool has_regx2() const noexcept
		{
			return m_RetType == RetType::RetRegx2;
		}

		bool has_regx1() const noexcept
		{
			return m_RetType == RetType::RetReg;
		}

		bool has_ret_regs() const noexcept
		{
			return has_regx1() || has_regx2();
		}

		bool has_ret_mem() const noexcept
		{
			return m_RetType == RetType::RetMem;
		}


		template<typename _Ty = asmjit::BaseReg>
		const _Ty& ret(bool low = false) const noexcept
		{
			return m_Ret[low ? 1 : 0].Reg.as<_Ty>();
		}


		template<typename _Ty = asmjit::BaseReg>
		const _Ty& ret_mem() const noexcept
		{
			return m_Ret[0].Reg.as<_Ty>();
		}


		bool has_this_ptr() const noexcept
		{
			return m_ContainThisPtr;
		}

		const asmjit::x86::Gp& this_() const noexcept
		{
			return m_Args[0].Reg.as<asmjit::x86::Gp>();
		}


		template<typename _Ty = asmjit::BaseReg>
		const _Ty& arg(size_t pos) const noexcept
		{
			return m_Args[pos + arg_offset()].Reg.as<_Ty>();
		}

		size_t arg_size(size_t pos) const noexcept
		{
			return m_Args[pos + arg_offset()].Size;
		}

		size_t total_size() const noexcept
		{
			return m_Args.size();
		}

		size_t arg_size() const noexcept
		{
			return total_size() - arg_offset();
		}

		bool has_args() const noexcept
		{
			return arg_size() > 0;
		}


		class Iterator
		{
		public:
			using iterator_type = std::vector<RegAndSize>::const_iterator;
			Iterator(const iterator_type& beg, const iterator_type& end)  noexcept : m_Begin(beg), m_End(end) { }

			auto begin() const noexcept { return m_Begin; }
			auto cbegin() const noexcept { return m_Begin; }

			auto end() const noexcept { return m_End; }
			auto cend() const noexcept { return m_End; }

		private:
			iterator_type m_Begin, m_End;
		};

		const Iterator args_iterator() const noexcept { return Iterator(m_Args.begin() + arg_offset(), m_Args.end()); }

	private:
		size_t arg_offset() const noexcept
		{
			return m_ContainThisPtr ? 1 : 0;
		}

		RegAndSize m_Ret[2];
		// first (and second if 64 bits) index is the return type
		// second (or third if 64 bits) index is the |this| pointer type if it exists
		// rests are the arguments type
		std::vector<RegAndSize> m_Args;

		bool m_ContainThisPtr{ };
		RetType m_RetType{ RetType::Void };
	};

	class SigBuilder
	{
	public:
		SigBuilder(const nlohmann::json& sig_data, std::string& err);

		const asmjit::FuncSignature& get_sig() const noexcept
		{
			return m_FuncSig;
		}

		std::unique_ptr<CallContext> load_args(asmjit::x86::Compiler& comp, TypeInfo& info);

	private:
		void SetCallConv(const std::string& callconv);

		void ManageFuncFrame(asmjit::FuncFrame& func_frame) const;

		asmjit::FuncSignatureBuilder m_FuncSig;

		/// <summary>
		/// 'return' section
		/// contains full types of return value
		/// </summary>
		std::vector<asmjit::TypeId> m_RetTypes;

		/// <summary>
		/// 'Arguments' section
		/// first pair is the underlying type
		/// second pair is for 'constness' of the type
		/// </summary>
		std::vector<std::pair<std::vector<asmjit::TypeId>, bool>> m_ArgTypes;

		const nlohmann::json& m_SigInfo;

		bool m_MutableThisPtr{ };
		TypeTable m_Types;
	};
}

SG_NAMESPACE_END;