#include "main.hpp"
#include "TechnicolourConfig.hpp"
#include "Hooks.hpp"
#include "SettingsViewController.hpp"
#include "questui/shared/QuestUI.hpp"
#include "custom-types/shared/register.hpp"

bool isColor0EventDataValue(int beatmapEventValue)
{
	if (beatmapEventValue != 1 && beatmapEventValue != 2 && beatmapEventValue != 3 && beatmapEventValue != 4 && beatmapEventValue != 0)
	{
		return beatmapEventValue == -1;
	}
	return true;
}

int pointerToSeed(void* ptr) {
    uintptr_t intValue = (uintptr_t) ptr;
    return (intValue * 200771) + (intValue % 300823);
}

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
static ModInfo trueModInfo; // This is used to store the actual ID of the mod

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getRawConfig() {
    static Configuration config(trueModInfo);
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(trueModInfo);
    return *logger;
}

TechnicolourConfig& getConfig() {
    static TechnicolourConfig config;
    return config;
}

void saveConfig() {
    getRawConfig().config.Clear();
    getRawConfig().config.SetObject();
    getConfig().save(getRawConfig().config);
    getRawConfig().Write();
    getLogger().info("Config saved");
}


// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    std::string modId = ID;

    // Hehheheheheh >:D

	srand(time(0));
	int randNo = rand() % 1;

	if (randNo == 0) {
		modId = "technicolor";
	}

    info.id = modId;
    info.version = VERSION;
    modInfo = info;

    trueModInfo = modInfo;
    trueModInfo.id = ID;
	
    getRawConfig().Load(); // Load the config file
    if(getRawConfig().config.HasMember("enabled")) {
        getLogger().info("Loading config file");
        getConfig().load(getRawConfig().config);
    }   else    {
        getLogger().info("Saving default config file");
        saveConfig();
    }

    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    getLogger().info("Updating force enable chroma");
    TechnicolourConfig::updateEnabled(getConfig().getEnabled());

    il2cpp_functions::Init();

    custom_types::Register::AutoRegister();

    installHooks();

    QuestUI::Register::RegisterGameplaySetupMenu<Technicolour::SettingsViewController*>(modInfo, "Technicolour", QuestUI::Register::MenuType::Solo | QuestUI::Register::MenuType::Online);
}