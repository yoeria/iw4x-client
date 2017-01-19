#define MAX_SOURCEFILES	64
#undef LoadMenu

namespace Components
{
	class Menus : public Component
	{
	public:
		Menus();
		~Menus();

#if defined(DEBUG) || defined(FORCE_UNIT_TESTS)
		const char* getName() { return "Menus"; };
#endif

		static void FreeEverything();

		static void Add(std::string menu);

	private:
		static std::unordered_map<std::string, Game::menuDef_t*> MenuList;
		static std::unordered_map<std::string, Game::MenuList*> MenuListList;
		static std::vector<std::string> CustomMenus;

		static Game::XAssetHeader MenuLoad(Game::XAssetType type, std::string filename);
		static Game::XAssetHeader MenuFileLoad(Game::XAssetType type, std::string filename);

		static Game::MenuList* LoadMenuList(Game::MenuList* menuList);
		static Game::MenuList* LoadScriptMenu(const char* menu);
		static std::vector<Game::menuDef_t*> LoadMenu(Game::menuDef_t* menudef);
		static std::vector<Game::menuDef_t*> LoadMenu(std::string file);

		static Game::script_t* LoadMenuScript(std::string name, std::string buffer);
		static int LoadMenuSource(std::string name, std::string buffer);

		static int ReserveSourceHandle();
		static bool IsValidSourceHandle(int handle);

		static Game::menuDef_t* ParseMenu(int handle);

		static void FreeMenuSource(int handle);

		static void FreeMenuList(Game::MenuList* menuList);
		static void FreeMenu(Game::menuDef_t* menudef);

		static void RemoveMenu(std::string menu);
		static void RemoveMenu(Game::menuDef_t* menudef);
		static void RemoveMenuList(std::string menuList);
		static void RemoveMenuList(Game::MenuList* menuList);

		static void OverrideMenu(Game::menuDef_t *menu);

		static bool IsMenuVisible(Game::UiContext *dc, Game::menuDef_t *menu);

		static void RemoveMenuFromContext(Game::UiContext *dc, Game::menuDef_t *menu);

		// Ugly!
		static int KeywordHash(char* key);
	};
}
