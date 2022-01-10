#include "STDInclude.hpp"

namespace Components
{
	SteamID Dedicated::PlayerGuids[18][2];
	Dvar::Var Dedicated::SVRandomMapRotation;

	bool Dedicated::IsEnabled()
	{
		static std::optional<bool> flag;

		if (!flag.has_value())
		{
			flag.emplace(Flags::HasFlag("dedicated"));
		}

		return flag.value();
	}

	void Dedicated::InitDedicatedServer()
	{
		static const char* fastfiles[7] =
		{
			"code_post_gfx_mp",
			"localized_code_post_gfx_mp",
			"ui_mp",
			"localized_ui_mp",
			"common_mp",
			"localized_common_mp",
			"patch_mp"
		};

		std::memcpy(reinterpret_cast<void*>(0x66E1CB0), &fastfiles, sizeof(fastfiles));
		Game::R_LoadGraphicsAssets();

		if (Dvar::Var("com_logFilter").get<bool>())
		{
			Utils::Hook::Nop(0x647466, 5); // 'dvar set' lines
			Utils::Hook::Nop(0x5DF4F2, 5); // 'sending splash open' lines
		}

		Utils::Hook::Call<void()>(0x4F84C0)();
	}

	void Dedicated::PostInitialization()
	{
		Command::Execute("exec autoexec.cfg");
		Command::Execute("onlinegame 1");
		Command::Execute("exec default_xboxlive.cfg");
		Command::Execute("xblive_rankedmatch 1");
		Command::Execute("xblive_privatematch 1");
		Command::Execute("xblive_privateserver 0");
		Command::Execute("xstartprivatematch");
		//Command::Execute("xstartlobby");
		Command::Execute("sv_network_fps 1000");
		Command::Execute("cl_maxpackets 125");
		Command::Execute("snaps 30");
		Command::Execute("com_maxfps 125");

		// Process command line?
		Utils::Hook::Call<void()>(0x60C3D0)();
	}

	__declspec(naked) void Dedicated::PostInitializationStub()
	{
		__asm
		{
			pushad
			call Dedicated::PostInitialization
			popad

			// Start Com_EvenLoop
			mov eax, 43D140h
			jmp eax
		}
	}

	void Dedicated::TransmitGuids()
	{
		std::string list = Utils::String::VA("%c", 20);

		for (int i = 0; i < 18; ++i)
		{
			if (Game::svs_clients[i].state >= 3)
			{
				list.append(Utils::String::VA(" %llX", Game::svs_clients[i].steamID));

				Utils::InfoString info(Game::svs_clients[i].connectInfoString);
				list.append(Utils::String::VA(" %llX", strtoull(info.get("realsteamId").data(), nullptr, 16)));
			}
			else
			{
				list.append(" 0 0");
			}
		}

		Game::SV_GameSendServerCommand(-1, 0, list.data());
	}

	void Dedicated::TimeWrapStub(Game::errorParm_t code, const char* message)
	{
		static bool partyEnable;
		static std::string mapname;

		partyEnable = Dvar::Var("party_enable").get<bool>();
		mapname = Dvar::Var("mapname").get<std::string>();

		Scheduler::Once([]()
		{
			Dvar::Var("party_enable").set(partyEnable);

			if (!partyEnable) // Time wrapping should not occur in party servers, but yeah...
			{
				if (mapname.empty()) mapname = "mp_rust";
				Command::Execute(Utils::String::VA("map %s", mapname.data()), false);
				mapname.clear();
			}
		});

		Game::Com_Error(code, message);
	}

	void Dedicated::RandomizeMapRotation()
	{
		auto rotation = Dvar::Var("sv_mapRotation").get<std::string>();

		const auto tokens = Utils::String::Explode(rotation, ' ');
		std::vector<std::pair<std::string, std::string>> mapRotationPair;

		for (auto i = 0u; i < (tokens.size() - 1); i += 2)
		{
			if (i + 1 >= tokens.size()) break;

			const auto& key = tokens[i];
			const auto& value = tokens[i + 1];
			mapRotationPair.push_back(std::make_pair(key, value));
		}

		const auto seed = Utils::Cryptography::Rand::GenerateInt();
		std::shuffle(std::begin(mapRotationPair), std::end(mapRotationPair), std::default_random_engine(seed));

		// Rebuild map rotation using the randomized key/values
		rotation.clear();
		for (auto j = 0u; j < mapRotationPair.size(); j++)
		{
			const auto& pair = mapRotationPair[j];
			rotation.append(pair.first);
			rotation.append(" ");
			rotation.append(pair.second);

			if (j != mapRotationPair.size() - 1)
				rotation.append(" ");
		}

		Dvar::Var("sv_mapRotationCurrent").set(rotation);
	}

	void Dedicated::MapRotate()
	{
		if (!Dedicated::IsEnabled() && Dvar::Var("sv_dontrotate").get<bool>())
		{
			Dvar::Var("sv_dontrotate").set(false);
			return;
		}

		if (Dvar::Var("party_enable").get<bool>() && Dvar::Var("party_host").get<bool>())
		{
			Logger::Print("Not performing map rotation as we are hosting a party!\n");
			return;
		}

		Logger::Print("Rotating map...\n");
		const auto mapRotation = Dvar::Var("sv_mapRotation").get<std::string>();

		// if nothing, just restart
		if (mapRotation.empty())
		{
			Logger::Print("No rotation defined, restarting map.\n");

			if (!Dvar::Var("sv_cheats").get<bool>())
			{
				Command::Execute(Utils::String::VA("map %s", Dvar::Var("mapname").get<const char*>()), true);
			}
			else
			{
				Command::Execute(Utils::String::VA("devmap %s", Dvar::Var("mapname").get<const char*>()), true);
			}

			return;
		}

		// First, check if the string contains nothing
		if (Dvar::Var("sv_mapRotationCurrent").get<std::string>().empty())
		{
			Logger::Print("Current map rotation has finished, reloading...\n");

			if (Dedicated::SVRandomMapRotation.get<bool>())
			{
				Logger::Print("Randomizing map rotation\n");
				Dedicated::RandomizeMapRotation();
			}
			else
			{
				Dvar::Var("sv_mapRotationCurrent").set(mapRotation);
			}
		}

		auto rotation = Dvar::Var("sv_mapRotationCurrent").get<std::string>();

		auto tokens = Utils::String::Explode(rotation, ' ');

		for (unsigned int i = 0; i < (tokens.size() - 1); i += 2)
		{
			if (i + 1 >= tokens.size())
			{
				Dvar::Var("sv_mapRotationCurrent").set("");
				Command::Execute("map_rotate", true);
				return;
			}

			std::string key = tokens[i];
			std::string value = tokens[i + 1];

			if (key == "map")
			{
				// Rebuild map rotation string
				rotation.clear();
				for (unsigned int j = (i + 2); j < tokens.size(); ++j)
				{
					if (j != (i + 2)) rotation += " ";
					rotation += tokens[j];
				}

				Dvar::Var("sv_mapRotationCurrent").set(rotation);

				Logger::Print("Loading new map: %s\n", value.data());
				Command::Execute(Utils::String::VA("map %s", value.data()), true);
				break;
			}
			else if (key == "gametype")
			{
				Logger::Print("Applying new gametype: %s\n", value.data());
				Dvar::Var("g_gametype").set(value);
			}
			else
			{
				Logger::Print("Unsupported maprotation key '%s', motherfucker!\n", key.data());
			}
		}
	}

	void Dedicated::Heartbeat()
	{
		int masterPort = Dvar::Var("masterPort").get<int>();
		const char* masterServerName = Dvar::Var("masterServerName").get<const char*>();

		Network::Address master(Utils::String::VA("%s:%u", masterServerName, masterPort));

		Logger::Print("Sending heartbeat to master: %s:%u\n", masterServerName, masterPort);
		Network::SendCommand(master, "heartbeat", "IW4");
	}

	__declspec(naked) void Dedicated::FrameStub()
	{
		__asm
		{
			pushad
			call Scheduler::FrameHandler
			popad

			push 5A8E80h
			retn
		}
	}

	Dedicated::Dedicated()
	{
		// Map rotation
		Utils::Hook::Set(0x4152E8, Dedicated::MapRotate);
		Dvar::Register<bool>("sv_dontrotate", false, Game::dvar_flag::DVAR_FLAG_CHEAT, "");
		Dvar::Register<bool>("com_logFilter", true, Game::dvar_flag::DVAR_FLAG_LATCHED, "Removes ~95% of unneeded lines from the log");

		if (Dedicated::IsEnabled() || ZoneBuilder::IsEnabled())
		{
			// Make sure all callbacks are handled
			Scheduler::OnFrame(Steam::SteamAPI_RunCallbacks);

			Dvar::Register<bool>("sv_lanOnly", false, Game::dvar_flag::DVAR_FLAG_NONE, "Don't act as node");

			Utils::Hook(0x60BE98, Dedicated::InitDedicatedServer, HOOK_CALL).install()->quick();

			Utils::Hook::Set<BYTE>(0x683370, 0xC3); // steam sometimes doesn't like the server

			Utils::Hook::Set<BYTE>(0x5B4FF0, 0xC3); // self-registration on party
			Utils::Hook::Set<BYTE>(0x426130, 0xC3); // other party stuff?

			Utils::Hook::Set<BYTE>(0x4D7030, 0xC3); // upnp stuff

			Utils::Hook::Set<BYTE>(0x4B0FC3, 0x04); // make CL_Frame do client packets, even for game state 9
			Utils::Hook::Set<BYTE>(0x4F5090, 0xC3); // init sound system (1)
			Utils::Hook::Set<BYTE>(0x507B80, 0xC3); // start render thread
			Utils::Hook::Set<BYTE>(0x4F84C0, 0xC3); // R_Init caller
			Utils::Hook::Set<BYTE>(0x46A630, 0xC3); // init sound system (2)
			Utils::Hook::Set<BYTE>(0x41FDE0, 0xC3); // Com_Frame audio processor?
			Utils::Hook::Set<BYTE>(0x41B9F0, 0xC3); // called from Com_Frame, seems to do renderer stuff
			Utils::Hook::Set<BYTE>(0x41D010, 0xC3); // CL_CheckForResend, which tries to connect to the local server constantly
			Utils::Hook::Set<BYTE>(0x62B6C0, 0xC3); // UI expression 'DebugPrint', mainly to prevent some console spam

			Utils::Hook::Set<BYTE>(0x468960, 0xC3); // some mixer-related function called on shutdown
			Utils::Hook::Set<BYTE>(0x60AD90, 0);    // masterServerName flags

			Utils::Hook::Nop(0x4DCEC9, 2);          // some check preventing proper game functioning
			Utils::Hook::Nop(0x507C79, 6);          // another similar bsp check
			Utils::Hook::Nop(0x414E4D, 6);          // unknown check in SV_ExecuteClientMessage (0x20F0890 == 0, related to client->f_40)
			Utils::Hook::Nop(0x4DCEE9, 5);          // some deinit renderer function
			Utils::Hook::Nop(0x59A896, 5);          // warning message on a removed subsystem
			Utils::Hook::Nop(0x4B4EEF, 5);          // same as above
			Utils::Hook::Nop(0x64CF77, 5);          // function detecting video card, causes Direct3DCreate9 to be called
			Utils::Hook::Nop(0x60BC52, 0x15);       // recommended settings check

			Utils::Hook::Nop(0x45148B, 5);          // Disable splash screen

			// do not trigger host migration, even if the server is a 'bad host'
			Utils::Hook::Set<BYTE>(0x626AA8, 0xEB);

			// isHost script call return 0
			Utils::Hook::Set<DWORD>(0x5DEC04, 0);

			// sv_network_fps max 1000, and uncheat
			Utils::Hook::Set<BYTE>(0x4D3C67, 0); // ?
			Utils::Hook::Set<DWORD>(0x4D3C69, 1000);

			// Manually register sv_network_fps
			Utils::Hook::Nop(0x4D3C7B, 5);
			Utils::Hook::Nop(0x4D3C8E, 5);
			*reinterpret_cast<Game::dvar_t**>(0x62C7C00) = Dvar::Register<int>("sv_network_fps", 1000, 20, 1000, Game::dvar_flag::DVAR_FLAG_NONE, "Number of times per second the server checks for net messages").get<Game::dvar_t*>();

			// r_loadForRenderer default to 0
			Utils::Hook::Set<BYTE>(0x519DDF, 0);

			// disable cheat protection on onlinegame
			Utils::Hook::Set<BYTE>(0x404CF7, 0x80);

			// some d3d9 call on error
			Utils::Hook::Set<BYTE>(0x508470, 0xC3);

			// stop saving a config_mp.cfg
			Utils::Hook::Set<BYTE>(0x60B240, 0xC3);

			// don't load the config
			Utils::Hook::Set<BYTE>(0x4B4D19, 0xEB);

			// Dedicated frame handler
			Utils::Hook(0x4B0F81, Dedicated::FrameStub, HOOK_CALL).install()->quick();

			// Intercept time wrapping
			Utils::Hook(0x62737D, Dedicated::TimeWrapStub, HOOK_CALL).install()->quick();
			//Utils::Hook::Set<DWORD>(0x62735C, 50'000); // Time wrap after 50 seconds (for testing - i don't want to wait 3 weeks)

			if (!ZoneBuilder::IsEnabled())
			{
				// Post initialization point
				Utils::Hook(0x60BFBF, Dedicated::PostInitializationStub, HOOK_JUMP).install()->quick();

				// Transmit custom data
				Scheduler::OnFrame([]()
				{
					static Utils::Time::Interval interval;
					if (interval.elapsed(10s))
					{
						interval.update();

						CardTitles::SendCustomTitlesToClients();
						//Clantags::SendClantagsToClients();
					}
				});

#ifdef USE_LEGACY_SERVER_LIST
				// Heartbeats
				Scheduler::Once(Dedicated::Heartbeat);
				Scheduler::OnFrame([]()
				{
					static Utils::Time::Interval interval;

					if (Dvar::Var("sv_maxclients").get<int>() > 0 && interval.elapsed(2min))
					{
						interval.update();
						Dedicated::Heartbeat();
					}
				});
#endif

				Dvar::OnInit([]()
				{
					Dedicated::SVRandomMapRotation = Dvar::Register<bool>("sv_randomMapRotation", false, Game::dvar_flag::DVAR_FLAG_SAVED, "Randomize map rotation when true");
					Dvar::Register<const char*>("sv_sayName", "^7Console", Game::dvar_flag::DVAR_FLAG_NONE, "The name to pose as for 'say' commands");
					Dvar::Register<const char*>("sv_motd", "", Game::dvar_flag::DVAR_FLAG_NONE, "A custom message of the day for servers");

					// Say command
					Command::AddSV("say", [](Command::Params* params)
					{
						if (params->length() < 2) return;

						std::string message = params->join(1);
						std::string name = Dvar::Var("sv_sayName").get<std::string>();

						if (!name.empty())
						{
							Game::SV_GameSendServerCommand(-1, 0, Utils::String::VA("%c \"%s: %s\"", 104, name.data(), message.data()));
							Game::Com_Printf(15, "%s: %s\n", name.data(), message.data());
						}
						else
						{
							Game::SV_GameSendServerCommand(-1, 0, Utils::String::VA("%c \"Console: %s\"", 104, message.data()));
							Game::Com_Printf(15, "Console: %s\n", message.data());
						}
					});

					// Tell command
					Command::AddSV("tell", [](Command::Params* params)
					{
						if (params->length() < 3) return;

						int client = atoi(params->get(1));
						std::string message = params->join(2);
						std::string name = Dvar::Var("sv_sayName").get<std::string>();

						if (!name.empty())
						{
							Game::SV_GameSendServerCommand(client, 0, Utils::String::VA("%c \"%s: %s\"", 104, name.data(), message.data()));
							Game::Com_Printf(15, "%s -> %i: %s\n", name.data(), client, message.data());
						}
						else
						{
							Game::SV_GameSendServerCommand(client, 0, Utils::String::VA("%c \"Console: %s\"", 104, message.data()));
							Game::Com_Printf(15, "Console -> %i: %s\n", client, message.data());
						}
					});

					// Sayraw command
					Command::AddSV("sayraw", [](Command::Params* params)
					{
						if (params->length() < 2) return;

						std::string message = params->join(1);
						Game::SV_GameSendServerCommand(-1, 0, Utils::String::VA("%c \"%s\"", 104, message.data()));
						Game::Com_Printf(15, "Raw: %s\n", message.data());
					});

					// Tellraw command
					Command::AddSV("tellraw", [](Command::Params* params)
					{
						if (params->length() < 3) return;

						int client = atoi(params->get(1));
						std::string message = params->join(2);
						Game::SV_GameSendServerCommand(client, 0, Utils::String::VA("%c \"%s\"", 104, message.data()));
						Game::Com_Printf(15, "Raw -> %i: %s\n", client, message.data());
					});

					// ! command
					Command::AddSV("!", [](Command::Params* params)
					{
						if (params->length() != 2) return;

						int client = -1;
						if (params->get(1) != "all"s)
						{
							client = atoi(params->get(1));

							if (client >= *reinterpret_cast<int*>(0x31D938C))
							{
								Game::Com_Printf(0, "Invalid player.\n");
								return;
							}
						}

						Game::SV_GameSendServerCommand(client, 0, Utils::String::VA("%c \"\"", 106));
					});
				});
			}
		}
		else
		{
			for (int i = 0; i < ARRAYSIZE(Dedicated::PlayerGuids); ++i)
			{
				Dedicated::PlayerGuids[i][0].bits = 0;
				Dedicated::PlayerGuids[i][1].bits = 0;
			}

			// Intercept server commands
			ServerCommands::OnCommand(20, [](Command::Params* params)
			{
				for (int client = 0; client < 18; client++)
				{
					Dedicated::PlayerGuids[client][0].bits = strtoull(params->get(2 * client + 1), nullptr, 16);
					Dedicated::PlayerGuids[client][1].bits = strtoull(params->get(2 * client + 2), nullptr, 16);

					if (Steam::Proxy::SteamFriends && Dedicated::PlayerGuids[client][1].bits != 0)
					{
						Steam::Proxy::SteamFriends->SetPlayedWith(Dedicated::PlayerGuids[client][1]);
					}
				}

				return true;
			});
		}

		Scheduler::OnFrame([]()
		{
			if (Dvar::Var("sv_running").get<bool>())
			{
				static Utils::Time::Interval interval;

				if (interval.elapsed(15s))
				{
					interval.update();
					Dedicated::TransmitGuids();
				}
			}
		});
	}

	Dedicated::~Dedicated()
	{

	}
}
