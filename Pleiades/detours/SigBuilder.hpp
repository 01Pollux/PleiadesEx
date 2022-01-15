#pragma once

#include <unordered_map>
#include <asmjit/asmjit.h>
#include <nlohmann/Json.hpp>

#include "TypeTable.hpp"

class DetourCallContext;
namespace detour_detail
{
	class TypeInfo
	{
		friend class SigBuilder;
		friend class DetourCallContext;
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

		[[nodiscard]] bool has_ret() const noexcept
		{
			return m_RetType != RetType::Void;
		}

		[[nodiscard]] bool has_regx2() const noexcept
		{
			return m_RetType == RetType::RetRegx2;
		}

		[[nodiscard]] bool has_regx1() const noexcept
		{
			return m_RetType == RetType::RetReg;
		}

		[[nodiscard]] bool has_ret_regs() const noexcept
		{
			return has_regx1() || has_regx2();
		}

		[[nodiscard]] bool has_ret_mem() const noexcept
		{
			return m_RetType == RetType::RetMem;
		}


		template<typename _Ty = asmjit::BaseReg>
		[[nodiscard]] const _Ty& ret(bool low = false) const noexcept
		{
			return m_Ret[low ? 1 : 0].Reg.as<_Ty>();
		}


		template<typename _Ty = asmjit::BaseReg>
		[[nodiscard]] const _Ty& ret_mem() const noexcept
		{
			return m_Ret[0].Reg.as<_Ty>();
		}


		[[nodiscard]] bool has_this_ptr() const noexcept
		{
			return m_ContainThisPtr;
		}

		[[nodiscard]] const asmjit::x86::Gp& this_() const noexcept
		{
			return m_Args[0].Reg.as<asmjit::x86::Gp>();
		}


		template<typename _Ty = asmjit::BaseReg>
		[[nodiscard]] const _Ty& arg(size_t pos) const noexcept
		{
			return m_Args[pos + arg_offset()].Reg.as<_Ty>();
		}

		[[nodiscard]] size_t arg_size(size_t pos) const noexcept
		{
			return m_Args[pos + arg_offset()].Size;
		}

		[[nodiscard]] size_t total_size() const noexcept
		{
			return m_Args.size();
		}

		[[nodiscard]] size_t arg_size() const noexcept
		{
			return total_size() - arg_offset();
		}

		[[nodiscard]] bool has_args() const noexcept
		{
			return arg_size() > 0;
		}


		class Iterator
		{
		public:
			using iterator_type = std::vector<RegAndSize>::const_iterator;
			Iterator(const iterator_type& beg, const iterator_type& end)  noexcept : m_Begin(beg), m_End(end) { }

			[[nodiscard]] auto begin() const noexcept { return m_Begin; }
			[[nodiscard]] auto cbegin() const noexcept { return m_Begin; }

			[[nodiscard]] auto end() const noexcept { return m_End; }
			[[nodiscard]] auto cend() const noexcept { return m_End; }

		private:
			iterator_type m_Begin, m_End;
		};

		[[nodiscard]] const Iterator args_iterator() const noexcept { return Iterator(m_Args.begin() + arg_offset(), m_Args.end()); }

	private:
		[[nodiscard]] size_t arg_offset() const noexcept
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

		[[nodiscard]] const asmjit::FuncSignature& get_sig() const noexcept
		{
			return m_FuncSig;
		}

		[[nodiscard]] std::unique_ptr<DetourCallContext> load_args(asmjit::x86::Compiler& comp, TypeInfo& info);

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
