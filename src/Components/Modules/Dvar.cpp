#include "STDInclude.hpp"

namespace Components
{
	Utils::Signal<Scheduler::Callback> Dvar::RegistrationSignal;
	const char* Dvar::ArchiveDvarPath = "userraw/archivedvars.cfg";

	Dvar::Var::Var(const std::string& dvarName) : Var()
	{
		this->dvar = Game::Dvar_FindVar(dvarName.data());

		if (!this->dvar)
		{
			// Quick-register the dvar
			Game::Dvar_SetStringByName(dvarName.data(), "");
			this->dvar = Game::Dvar_FindVar(dvarName.data());
		}
	}

	template <> Game::dvar_t* Dvar::Var::get()
	{
		return this->dvar;
	}

	template <> char* Dvar::Var::get()
	{
		if (this->dvar && this->dvar->type == Game::dvar_type::DVAR_TYPE_STRING && this->dvar->current.string)
		{
			return const_cast<char*>(this->dvar->current.string);
		}

		return const_cast<char*>("");
	}

	template <> const char* Dvar::Var::get()
	{
		return this->get<char*>();
	}

	template <> int Dvar::Var::get()
	{
		if (this->dvar && this->dvar->type == Game::dvar_type::DVAR_TYPE_INT || this->dvar->type == Game::dvar_type::DVAR_TYPE_ENUM)
		{
			return this->dvar->current.integer;
		}

		return 0;
	}

	template <> unsigned int Dvar::Var::get()
	{
		return static_cast<unsigned int>(this->get<int>());
	}

	template <> float Dvar::Var::get()
	{
		if (this->dvar && this->dvar->type == Game::dvar_type::DVAR_TYPE_FLOAT)
		{
			return this->dvar->current.value;
		}

		return 0;
	}

	template <> float* Dvar::Var::get()
	{
		static float val[4] = { 0 };

		if (this->dvar && (this->dvar->type == Game::dvar_type::DVAR_TYPE_FLOAT_2 || this->dvar->type == Game::dvar_type::DVAR_TYPE_FLOAT_3 || this->dvar->type == Game::dvar_type::DVAR_TYPE_FLOAT_4))
		{
			return this->dvar->current.vector;
		}

		return val;
	}

	template <> bool Dvar::Var::get()
	{
		if (this->dvar && this->dvar->type == Game::dvar_type::DVAR_TYPE_BOOL)
		{
			return this->dvar->current.enabled;
		}

		return false;
	}

	template <> std::string Dvar::Var::get()
	{
		return this->get<const char*>();
	}

	void Dvar::Var::set(char* string)
	{
		this->set(const_cast<const char*>(string));
	}

	void Dvar::Var::set(const char* string)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_STRING);
		if (this->dvar)
		{
			Game::Dvar_SetString(this->dvar, string);
		}
	}

	void Dvar::Var::set(const std::string& string)
	{
		this->set(string.data());
	}

	void Dvar::Var::set(int integer)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_INT);
		if (this->dvar)
		{
			Game::Dvar_SetInt(this->dvar, integer);
		}
	}

	void Dvar::Var::set(float value)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_FLOAT);
		if (this->dvar)
		{
			Game::Dvar_SetFloat(this->dvar, value);
		}
	}

	void Dvar::Var::set(bool enabled)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_BOOL);
		if (this->dvar)
		{
			Game::Dvar_SetBool(this->dvar, enabled);
		}
	}

	void Dvar::Var::setRaw(int integer)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_INT);
		if (this->dvar)
		{
			this->dvar->current.integer = integer;
			this->dvar->latched.integer = integer;
		}
	}

	void Dvar::Var::setRaw(float value)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_FLOAT);
		if (this->dvar)
		{
			this->dvar->current.value = value;
			this->dvar->latched.value = value;
		}
	}

	void Dvar::Var::setRaw(bool enabled)
	{
		assert(this->dvar->type == Game::DVAR_TYPE_BOOL);
		if (this->dvar)
		{
			this->dvar->current.enabled = enabled;
			this->dvar->latched.enabled = enabled;
		}
	}

	template<> static Dvar::Var Dvar::Register(const char* name, bool value, Dvar::Flag flag, const char* description)
	{
		return Game::Dvar_RegisterBool(name, value, flag.val, description);
	}

	template<> static Dvar::Var Dvar::Register(const char* name, const char* value, Dvar::Flag flag, const char* description)
	{
		return Game::Dvar_RegisterString(name, value, flag.val, description);
	}

	template<> static Dvar::Var Dvar::Register(const char* name, int value, int min, int max, Dvar::Flag flag, const char* description)
	{
		return Game::Dvar_RegisterInt(name, value, min, max, flag.val, description);
	}

	template<> static Dvar::Var Dvar::Register(const char* name, float value, float min, float max, Dvar::Flag flag, const char* description)
	{
		return Game::Dvar_RegisterFloat(name, value, min, max, flag.val, description);
	}

	void Dvar::OnInit(Utils::Slot<Scheduler::Callback> callback)
	{
		Dvar::RegistrationSignal.connect(callback);
	}

	void Dvar::ResetDvarsValue()
	{
		if (!Utils::IO::FileExists(Dvar::ArchiveDvarPath))
			return

		Command::Execute("exec archivedvars.cfg", true);
		// Clean up
		Utils::IO::RemoveFile(Dvar::ArchiveDvarPath);
	}

	Game::dvar_t* Dvar::RegisterName(const char* name, const char* /*default*/, Game::dvar_flag flag, const char* description)
	{
		// Run callbacks
		Dvar::RegistrationSignal();

		// Name watcher
		Scheduler::OnFrame([]()
		{
			static std::string lastValidName = "Unknown Soldier";
			std::string name = Dvar::Var("name").get<char*>();

			// Don't perform any checks if name didn't change
			if (name == lastValidName) return;

			std::string saneName = TextRenderer::StripAllTextIcons(TextRenderer::StripColors(Utils::String::Trim(name)));
			if (saneName.size() < 3 || (saneName[0] == '[' && saneName[1] == '{'))
			{
				Logger::Print("Username '%s' is invalid. It must at least be 3 characters long and not appear empty!\n", name.data());
				Dvar::Var("name").set(lastValidName);
			}
			else
			{
				lastValidName = name;
				Friends::UpdateName();
			}
		}, true);

		std::string username = "Unknown Soldier";

		if (Steam::Proxy::SteamFriends)
		{
			const char* steamName = Steam::Proxy::SteamFriends->GetPersonaName();

			if (steamName && !std::string(steamName).empty())
			{
				username = steamName;
			}
		}

		return Dvar::Register<const char*>(name, username.data(), Dvar::Flag(flag | Game::dvar_flag::DVAR_FLAG_SAVED).val, description).get<Game::dvar_t*>();
	}

	Game::dvar_t* Dvar::SetFromStringByNameSafeExternal(const char* dvar, const char* value)
	{
		static const char* exceptions[] =
		{
			"ui_showEndOfGame",
			"systemlink",
			"splitscreen",
			"onlinegame",
			"party_maxplayers",
			"xblive_privateserver",
			"xblive_rankedmatch",
			"ui_mptype",
		};

		for (int i = 0; i < ARRAYSIZE(exceptions); ++i)
		{
			if (Utils::String::ToLower(dvar) == Utils::String::ToLower(exceptions[i]))
			{
				return Game::Dvar_SetFromStringByName(dvar, value);
			}
		}

		return Dvar::SetFromStringByNameExternal(dvar, value);
	}

	Game::dvar_t* Dvar::SetFromStringByNameExternal(const char* dvar, const char* value)
	{
		return Game::Dvar_SetFromStringByNameFromSource(dvar, value, Game::DvarSetSource::DVAR_SOURCE_EXTERNAL);
	}

	void Dvar::SaveArchiveDvar(const Game::dvar_t* var)
	{
		if (!Utils::IO::FileExists(Dvar::ArchiveDvarPath))
		{
			Utils::IO::WriteFile(Dvar::ArchiveDvarPath,
				"// generated by IW4x, do not modify\n");
		}

		Utils::IO::WriteFile(Dvar::ArchiveDvarPath,
			Utils::String::VA("seta %s \"%s\"\n", var->name, Game::Dvar_DisplayableValue(var)), true);
	}

	void Dvar::DvarSetFromStringByNameStub(const char* dvarName, const char* value)
	{
		// Save the dvar original value if it has the archive flag
		const auto* dvar = Game::Dvar_FindVar(dvarName);
		if (dvar != nullptr && dvar->flags & Game::dvar_flag::DVAR_FLAG_SAVED)
		{
			Dvar::SaveArchiveDvar(dvar);
		}

		Utils::Hook::Call<void(const char*, const char*)>(0x4F52E0)(dvarName, value);
	}

	Dvar::Dvar()
	{
		// set flags of cg_drawFPS to archive
		Utils::Hook::Or<BYTE>(0x4F8F69, Game::dvar_flag::DVAR_FLAG_SAVED);

		// un-cheat camera_thirdPersonCrosshairOffset and add archive flags
		Utils::Hook::Xor<BYTE>(0x447B41, Game::dvar_flag::DVAR_FLAG_CHEAT | Game::dvar_flag::DVAR_FLAG_SAVED);
		
		// un-cheat cg_fov and add archive flags
		Utils::Hook::Xor<BYTE>(0x4F8E35, Game::dvar_flag::DVAR_FLAG_CHEAT | Game::dvar_flag::DVAR_FLAG_SAVED);
		
		// un-cheat cg_fovscale and add archive flags
		Utils::Hook::Xor<BYTE>(0x4F8E68, Game::dvar_flag::DVAR_FLAG_CHEAT | Game::dvar_flag::DVAR_FLAG_SAVED);

		// un-cheat cg_debugInfoCornerOffset and add archive flags
		Utils::Hook::Xor<BYTE>(0x4F8FC2, Game::dvar_flag::DVAR_FLAG_CHEAT | Game::dvar_flag::DVAR_FLAG_SAVED);

		// remove archive flags for cg_hudchatposition
		Utils::Hook::Xor<BYTE>(0x4F9992, Game::dvar_flag::DVAR_FLAG_SAVED);

		// remove write protection from fs_game
		Utils::Hook::Xor<DWORD>(0x6431EA, Game::dvar_flag::DVAR_FLAG_WRITEPROTECTED);

		// set cg_fov max to 90.0
		// ...120 because of V2
		static float cgFov90 = 120.0f;
		Utils::Hook::Set<float*>(0x4F8E28, &cgFov90);

		// set max volume to 1
		static float volume = 1.0f;
		Utils::Hook::Set<float*>(0x408078, &volume);

		// Uncheat ui_showList
		Utils::Hook::Xor<BYTE>(0x6310DC, Game::dvar_flag::DVAR_FLAG_CHEAT);

		// Uncheat ui_debugMode
		Utils::Hook::Xor<BYTE>(0x6312DE, Game::dvar_flag::DVAR_FLAG_CHEAT);

		// Hook dvar 'name' registration
		Utils::Hook(0x40531C, Dvar::RegisterName, HOOK_CALL).install()->quick();

		// un-cheat safeArea_* and add archive flags
		Utils::Hook::Xor<INT>(0x42E3F5, Game::dvar_flag::DVAR_FLAG_READONLY | Game::dvar_flag::DVAR_FLAG_SAVED); //safeArea_adjusted_horizontal
		Utils::Hook::Xor<INT>(0x42E423, Game::dvar_flag::DVAR_FLAG_READONLY | Game::dvar_flag::DVAR_FLAG_SAVED); //safeArea_adjusted_vertical
		Utils::Hook::Xor<BYTE>(0x42E398, Game::dvar_flag::DVAR_FLAG_CHEAT | Game::dvar_flag::DVAR_FLAG_SAVED); //safeArea_horizontal
		Utils::Hook::Xor<BYTE>(0x42E3C4, Game::dvar_flag::DVAR_FLAG_CHEAT | Game::dvar_flag::DVAR_FLAG_SAVED); //safeArea_vertical

		// Don't allow setting cheat protected dvars via menus
/* 		Utils::Hook(0x63C897, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x63CA96, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x63CDB5, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x635E47, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
 */
		// SetDvar
		Utils::Hook(0x63444C, Dvar::SetFromStringByNameSafeExternal, HOOK_CALL).install()->quick();

		// Slider
		Utils::Hook(0x636159, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x636189, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x6364EA, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();

		Utils::Hook(0x636207, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x636608, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();
		Utils::Hook(0x636695, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();

		// Entirely block setting cheat dvars internally without sv_cheats
		//Utils::Hook(0x4F52EC, Dvar::SetFromStringByNameExternal, HOOK_CALL).install()->quick();

		// Hook Dvar_SetFromStringByName inside CG_SetClientDvarFromServer so we can reset dvars when the player leaves the server
		Utils::Hook(0x59386A, Dvar::DvarSetFromStringByNameStub, HOOK_CALL).install()->quick();

		// If the game closed abruptly, the dvars would not have been restored

		Dvar::OnInit([]
		{
			Dvar::ResetDvarsValue();
		});
	}

	Dvar::~Dvar()
	{
		Dvar::RegistrationSignal.clear();
		Utils::IO::RemoveFile(Dvar::ArchiveDvarPath);
	}
}
