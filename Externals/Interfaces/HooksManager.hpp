#pragma once

#include "InterfacesSys.hpp"
#include "User/EnumClass.hpp"
#include "User/IntPtr.hpp"
#include "HookArgs.hpp"

_STD_BEGIN;

template<typename>
class function;

_STD_END;


SG_NAMESPACE_BEGIN;

enum class HookOrder : char8_t
{
	ReservedFirst,
	VeryEarly,
	Early,
	Any,
	Late,
	VeryLate,
	ReservedLast
};

enum class HookRes : uint32_t
{
	Ignored,			// no action, ignore other flags
	ChangedParams,		// change args for hook
	ChangedReturn,		// change return value
	IgnorePostReturn,	// ignore post hook's return value
	BreakLoop,			// break callbacks loop
	DontCall,			// dont call the actual function
	SkipPost,			// dont call the post hooks
	//RerunLoops		// break the loop and re-call the previously called function

	_Highest_Enum,
};
using MHookRes = BitMask<HookRes>;

static constexpr const char* Interface_DetoursManager = "IDetoursManager";

class IHookInstance : public IInterface
{
public:
	using HookID = uint32_t;
	using CallbackType = std::function<MHookRes(PassRet*, PassArgs*)>;
	static constexpr HookID InvalidId = std::numeric_limits<SG::IHookInstance::HookID>::max();

	/// <summary>
	/// register a callback
	/// </summary>
	/// <param name="order">insertion order</param>
	/// <param name="callback">callback function</param>
	/// <return>id to the callback</return>
	virtual HookID AddCallback(bool post, HookOrder order, const CallbackType& callback) abstract;

	/// <summary>
	/// remove a callback
	/// </summary>
	/// <param name="order">insertion order</param>
	/// <param name="id">callback id</param>
	virtual void RemoveCallback(bool post, HookID id) noexcept abstract;

	/// <summary>
	/// get the last HookRes, only valid during post callbacks
	/// </summary>
	virtual MHookRes GetLastResults() noexcept abstract;

	/// <summary>
	/// get the original detoured function
	/// </summary>
	virtual void* GetFunction() noexcept abstract;
};

class IGameData;
class IDetoursManager : public IInterface
{
public:
	/// <summary>
	/// Load a hook from Plugin's gamedata, fallback to the main gamedata if it doesn't exists
	/// if 'pThis' is nullptr, this function will ignore 'virtual' section in 'gamedata'
	/// if 'lookupKey' and 'pAddr' aren't nullptr, this function will ignore 'pThis', ('virtual', 'address' and 'name') keys in 'gamedata'
	/// </summary>
	/// <param name="hookName">Entry name in config, required for function's signature</param>
	/// <param name="pThis">this pointer for virtual functions</param>
	/// <param name="pPlugin">Plugin owner</param>
	/// <param name="lookupKey">key to be used for searching for pre-exisiting hooks</param>
	/// <param name="pAddr">address of pre-existing function</param>
	virtual IHookInstance* LoadHook(const std::vector<std::string>& keys, const char* hookName, void* pThis, IGameData* gamedata, IntPtr lookupKey = nullptr, IntPtr pAddr = nullptr) abstract;

	/// <summary>
	/// Load a hook from Plugin's gamedata, fallback to the main gamedata if it doesn't exists
	/// if 'lookupKey' and 'pAddr' aren't nullptr, this function will ignore ('virtual', 'address' and 'name') keys in 'gamedata'
	/// </summary>
	/// <param name="hookName">Entry name in config</param>
	/// <param name="pPlugin">Plugin owner</param>
	IHookInstance* LoadHook(const std::vector<std::string>& keys, const char* hookName, IGameData* gamedata, IntPtr lookupKey = nullptr, IntPtr pAddr = nullptr)
	{
		return LoadHook(keys, hookName, nullptr, gamedata, lookupKey, pAddr);
	}

	/// <summary>
	/// decrements hook's reference counter, will be unloaded once it reaches zero
	/// </summary>
	virtual void ReleaseHook(IHookInstance*& hookInst) abstract;
};

class ISHookToken
{
public:
	ISHookToken() = default;
	ISHookToken(IHookInstance* inst) : m_Instance(inst) { }

	void attach(bool post, HookOrder order, const IHookInstance::CallbackType& callback)
	{
		m_ID = m_Instance->AddCallback(post, order, callback);
		m_IsPost = post;
	}

	void detach()
	{
		m_Instance->RemoveCallback(m_IsPost, m_ID);
		m_ID = IHookInstance::InvalidId;
	}

	operator bool() const noexcept
	{
		return m_ID != IHookInstance::InvalidId;
	}

	IHookInstance*& instance() noexcept
	{
		return m_Instance;
	}

	const IHookInstance* instance() const noexcept
	{
		return m_Instance;
	}

private:
	IHookInstance* m_Instance{ };
	IHookInstance::HookID m_ID{ IHookInstance::InvalidId };
	bool m_IsPost{ };
};

class IMHookToken
{
public:
	IMHookToken() = default;
	IMHookToken(IHookInstance* inst) : m_Instance(inst) { }

	void attach(bool post, HookOrder order, const IHookInstance::CallbackType& callback)
	{
		m_IDs.push_back({ m_Instance->AddCallback(post, order, callback), post});
	}

	void detach(size_t i = 1)
	{
		for (; i > 0; --i)
		{
			auto id_and_state = m_IDs.back();
			m_Instance->RemoveCallback(id_and_state.second, id_and_state.first);
			m_IDs.pop_back();
		}
	}

	void detach_all()
	{
		for (auto id_and_state : m_IDs)
			m_Instance->RemoveCallback(id_and_state.second, id_and_state.first);
		m_IDs.clear();
	}

	operator bool() const noexcept
	{
		return m_IDs.size() > 0;
	}

	size_t size() const noexcept
	{
		return m_IDs.size();
	}

	void reserve(size_t cap)
	{
		m_IDs.reserve(cap);
	}

private:
	IHookInstance* m_Instance{ };
	std::vector<std::pair<IHookInstance::HookID, bool>> m_IDs;
};

SG_NAMESPACE_END;