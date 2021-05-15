#pragma once

namespace Components
{
	class Maps : public Component
	{
	public:
		class UserMapContainer
		{
		public:
			UserMapContainer() : wasFreed(false), hash(0) {}
			UserMapContainer(const std::string& _mapname) : wasFreed(false), mapname(_mapname)
			{
				ZeroMemory(&this->searchPath, sizeof this->searchPath);
				this->hash = Maps::GetUsermapHash(this->mapname);
				Game::UI_UpdateArenas();
			}

			~UserMapContainer()
			{
				this->freeIwd();
				this->clear();
			}

			unsigned int getHash() { return this->hash; }
			std::string getName() { return this->mapname; }
			bool isValid() { return !this->mapname.empty(); }
			void clear()
			{
				bool wasValid = this->isValid();
				this->mapname.clear();
				if (wasValid) Game::UI_UpdateArenas();
			}

			void loadIwd();
			void freeIwd();

			void reloadIwd();

			void handlePackfile(void* packfile);

		private:
			bool wasFreed;
			unsigned int hash;
			std::string mapname;
			Game::searchpath_t searchPath;
		};

		Maps();
		~Maps();

		static void HandleAsSPMap();
		static void AddDependency(const std::string& expression, const std::string& zone);

		static std::pair<std::string, std::string> GetTeamsForMap(const std::string& map);
		static std::vector<std::string> GetDependenciesForMap(const std::string& map);

		static std::string CurrentMainZone;
		static const char* UserMapFiles[4];

		static bool CheckMapInstalled(const char* mapname, bool error = false, bool dlcIsTrue = false);

		static UserMapContainer* GetUserMap();
		static unsigned int GetUsermapHash(const std::string& map);

		static Game::XAssetEntry* GetAssetEntryPool();
		static bool IsCustomMap();
		static bool IsUserMap(const std::string& mapname);

	private:
		class DLC
		{
		public:
			int index;
			std::string name;
			std::vector<std::string> maps;
		};

		static bool SPMap;
		static UserMapContainer UserMap;
		static std::vector<DLC> DlcPacks;

		static std::vector<std::pair<std::string, std::string>> DependencyList;
		static std::vector<std::string> CurrentDependencies;

		static void GetBSPName(char* buffer, size_t size, const char* format, const char* mapname);
		static void LoadAssetRestrict(Game::XAssetType type, Game::XAssetHeader asset, const std::string& name, bool* restrict);
		static void LoadMapZones(Game::XZoneInfo *zoneInfo, unsigned int zoneCount, int sync);
		static void UnloadMapZones(Game::XZoneInfo *zoneInfo, unsigned int zoneCount, int sync);

		static void OverrideMapEnts(Game::MapEnts* ents);

		static int IgnoreEntityStub(const char* entity);

		static Game::G_GlassData* GetWorldData();
		static void GetWorldDataStub();

		static void LoadRawSun();

		static void AddDlc(DLC dlc);
		static void UpdateDlcStatus();

		static void PrepareUsermap(const char* mapname);
		static void SpawnServerStub();
		static void LoadMapLoadscreenStub();

		static int TriggerReconnectForMap(Game::msg_t* msg, const char* mapname);
		static void RotateCheckStub();
		static void LoadNewMapCommand(char* buffer, size_t size, const char* format, const char* mapname, const char* gametype);

		static const char* LoadArenaFileStub(const char* name, char* buffer, int size);

		static void HideModel();
		static void HideModelStub();

		static Game::dvar_t* GetDistortionDvar();
		static void SetDistortionStub();

		static Game::dvar_t* GetSpecularDvar();
		static void SetSpecularStub1();
		static void SetSpecularStub2();
		static void G_SpawnTurretHook(Game::gentity_s* ent, int unk, int unk2);
		static bool SV_SetTriggerModelHook(Game::gentity_s* ent);
		static int16 CM_TriggerModelBounds(int brushModelPointer, Game::Bounds* bounds);
	};
}
