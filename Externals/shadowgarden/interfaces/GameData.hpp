#pragma once

#include "InterfacesSys.hpp"
#include "User/IntPtr.hpp"
#include <optional>
#include <vector>

SG_NAMESPACE_BEGIN;

class IGameData : public IInterface
{
public:
	/// <summary>
	/// Insert files for any of IGameData's functions to access
	/// directory is relative to the calling dll's owner path
	/// eg:
	/// IGameData::PushFiles({ "/MyNewPath/MyCustomGameData" }) will allow IGameData to seek 
	/// '<dll_owner>/MyNewPath/MyCustomGameData.<host_name>.<???>.json where '???' can be 'signatures' etc
	/// </summary>
	virtual void PushFiles(std::initializer_list<const char*> files) abstract;

	/// <summary>
	/// Load a signature in gamedata's files
	/// default files are 'Pleiades.<host_name>.signatures.json' and '<dll_owner>.<host_name>.signatures.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="keys">list of keys to access</param>
	/// <param name="signame">signature name</param>
	virtual IntPtr ReadSignature(const std::vector<std::string>& keys, const std::string& signame) abstract;

	/// <summary>
	/// Load an address in gamedata's files
	/// default files are 'Pleiades.<host_name>.addresses.json' and '<dll_owner>.<host_name>.addresses.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="keys">list of keys to access</param>
	/// <param name="addrname">address name</param>
	virtual IntPtr ReadAddress(const std::vector<std::string>& keys, const std::string& addrname) abstract;

	/// <summary>
	/// Load a virtual function's offset in gamedata's files
	/// default files are 'Pleiades.<host_name>.virtuals.json' and '<dll_owner>.<host_name>.virtuals.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="keys">list of keys to access</param>
	/// <param name="func_name">function name</param>
	/// <param name="pThis">|this| pointer to access virtual function</param>
	virtual IntPtr ReadVirtual(const std::vector<std::string>& keys, const std::string& func_name, IntPtr pThis) abstract;

	/// <summary>
	/// Load an offset in gamedata's files
	/// default files are 'Pleiades.<host_name>.offsets.json' and '<dll_owner>.<host_name>.offsets.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="keys">list of keys to access</param>
	/// <param name="name">offset name</param>
	virtual std::optional<int> ReadOffset(const std::vector<std::string>& keys, const std::string& name) abstract;

	/// <summary>
	/// Load a signature in gamedata's files
	/// default files are 'Pleiades.<host_name>.signatures.json' and '<dll_owner>.<host_name>.signatures.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="signame">signature name</param>
	IntPtr ReadSignature(const std::string& func_name)
	{
		return ReadSignature({ }, func_name);
	}

	/// <summary>
	/// Load an address in gamedata's files
	/// default files are 'Pleiades.<host_name>.addresses.json' and '<dll_owner>.<host_name>.addresses.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="addrname">address name</param>
	IntPtr ReadAddress(const std::string& func_name)
	{
		return ReadAddress({ }, func_name);
	}

	/// <summary>
	/// Load a virtual function's offset in gamedata's files
	/// default files are 'Pleiades.<host_name>.virtuals.json' and '<dll_owner>.<host_name>.virtuals.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="func_name">function name</param>
	/// <param name="pThis">|this| pointer to access virtual function</param>
	IntPtr ReadVirtual(const std::string& func_name, IntPtr pThis)
	{
		return ReadVirtual({ }, func_name, pThis);
	}

	/// <summary>
	/// Load an offset in gamedata's files
	/// default files are 'Pleiades.<host_name>.offsets.json' and '<dll_owner>.<host_name>.offsets.json'
	/// Use 'IGameData::PushFiles' to insert more directories to seek
	/// </summary>
	/// <param name="name">offset name</param>
	std::optional<int> ReadOffset(const std::string& func_name)
	{
		return ReadOffset({ }, func_name);
	}
};

SG_NAMESPACE_END