#include "Hooks.hpp"
#include "main.hpp"
#include "GradientController.hpp"
#include "SettingsViewController.hpp"
#include "TechnicolourController.hpp"
#include "GradientControllerUpdate.hpp"
#include "TechnicolourConfig.hpp"

#include "GlobalNamespace/BaseNoteVisuals.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/ColorType.hpp"
#include "GlobalNamespace/ObstacleController.hpp"
#include "GlobalNamespace/ObstacleData.hpp"
#include "GlobalNamespace/BeatmapObjectCallbackController.hpp"
#include "GlobalNamespace/LightSwitchEventEffect.hpp"
#include "GlobalNamespace/BeatmapEventData.hpp"
#include "GlobalNamespace/BloomPrePassBackgroundColorsGradientFromColorSchemeColors.hpp"
#include "GlobalNamespace/BloomPrePassBackgroundColorsGradient.hpp"
#include "GlobalNamespace/BloomPrePassBackgroundColorsGradient_Element.hpp"
using namespace GlobalNamespace;

#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "System/Object.hpp"

#include "chroma/shared/NoteAPI.hpp"
#include "chroma/shared/BombAPI.hpp"
#include "chroma/shared/ObstacleAPI.hpp"
#include "chroma/shared/LightAPI.hpp"


static std::unordered_map<int, int> latestLightEventDataValues;
static BloomPrePassBackgroundColorsGradientFromColorSchemeColors* currentBloomColours = nullptr;

std::unordered_map<int, int>& getLatestLightEventDataValues() {
    return latestLightEventDataValues;
}

void setGradientColours(Sombrero::FastColor colorLeft, Sombrero::FastColor colorRight) {
    if(!currentBloomColours) {
        return;
    }

    BloomPrePassBackgroundColorsGradient* gradient = currentBloomColours->bloomPrePassBackgroundColorsGradient;

    if (!getConfig().disableGradientBackground)
    {
        gradient->elements->values[0]->color = colorLeft * currentBloomColours->groundColorIntensity;
        gradient->elements->values[1]->color = colorLeft * currentBloomColours->groundColorIntensity;
        gradient->elements->values[5]->color = colorRight * currentBloomColours->skyColorIntensity;
        gradient->UpdateGradientTexture();
    }
}

MAKE_HOOK_MATCH(BaseNoteVisuals_HandleNoteControllerDidInit, &BaseNoteVisuals::HandleNoteControllerDidInit, void, BaseNoteVisuals* self, NoteControllerBase* noteController) {
    BaseNoteVisuals_HandleNoteControllerDidInit(self, noteController);

    // Return if TRUE_RANDOM notes are disabled
    if(!(getConfig().blocksStyle != TechnicolourStyle::GRADIENT && getConfig().blocksStyle != TechnicolourStyle::OFF)) {
        return;
    }

    NoteData* noteData = noteController->get_noteData();

    Sombrero::FastColor color = TechnicolourController::getTechnicolour(noteData->colorType == ColorType::ColorA, noteData->time + noteController->GetInstanceID(), getConfig().blocksStyle);

    if(noteData->colorType == ColorType::None) {
        Chroma::BombAPI::setBombColorSafe(reinterpret_cast<BombNoteController*>(self->noteController), color);
    }   else    {
        Chroma::NoteAPI::setNoteColorSafe(self->noteController, color);
    }
}

MAKE_HOOK_MATCH(ObstacleController_Init, &ObstacleController::Init, void, ObstacleController* self, ObstacleData* obstacleData, float worldRotation, UnityEngine::Vector3 startPos, UnityEngine::Vector3 midPos, UnityEngine::Vector3 endPos, float move1Duration, float move2Duration, float singleLineWidth, float height) {
    ObstacleController_Init(self, obstacleData, worldRotation, startPos, midPos, endPos, move1Duration, move2Duration, singleLineWidth, height);
    // Return if TRUE_RANDOM walls are disabled
    if(!(getConfig().wallsStyle != TechnicolourStyle::GRADIENT && getConfig().wallsStyle != TechnicolourStyle::OFF)) {
        return;
    }


    UnityEngine::Color color = TechnicolourController::getTechnicolour(true, obstacleData->time + self->GetInstanceID(), getConfig().wallsStyle);
    Chroma::ObstacleAPI::setObstacleColorSafe(self, color);
}

MAKE_HOOK_MATCH(BeatmapObjectCallbackController_Start, &BeatmapObjectCallbackController::Start, void, BeatmapObjectCallbackController* self) {
    BeatmapObjectCallbackController_Start(self);

    if(getConfig().getEnabled()) {
        getLogger().info("Initialising gradient controller");
        UnityEngine::GameObject* gameObj = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("Chroma_TechnicolorController"));
        Technicolour::GradientControllerUpdate* updator = gameObj->AddComponent<Technicolour::GradientControllerUpdate*>();
        GradientController& gradientController = updator->gradientController;
        gradientController.match = getConfig().desync;
        gradientController.mismatchSpeedOffset = getConfig().desync ? 0.0f : 0.5f;

        gradientController.initialiseGradients();
    }
}

MAKE_HOOK_MATCH(LightSwitchEventEffect_HandleBeatmapObjectCallbackControllerBeatmapEventDidTrigger, &LightSwitchEventEffect::HandleBeatmapObjectCallbackControllerBeatmapEventDidTrigger, void, LightSwitchEventEffect* self, BeatmapEventData* beatmapEventData) {
    latestLightEventDataValues[self->lightsID] = beatmapEventData->value;
    if (getConfig().lightsStyle != TechnicolourStyle::GRADIENT && getConfig().lightsStyle != TechnicolourStyle::OFF && getConfig().getEnabled() && beatmapEventData->type == self->event && beatmapEventData->value > 0 && beatmapEventData->value <= 7)
    {
        bool warm = !isColor0EventDataValue(beatmapEventData->value);

        if (getConfig().lightsGrouping == TechnicolourLightsGrouping::ISOLATED)
        {
            auto lights = Chroma::LightAPI::getLightsSafe(self);
            if(!lights) {
                getLogger().warning("No lights found for LightSwitchEventEffect");
                return;
            }

            for (ILightWithId* light : lights.value())
            {
                Sombrero::FastColor color = TechnicolourController::getTechnicolour(warm, beatmapEventData->time + reinterpret_cast<System::Object*>(light)->GetHashCode(), getConfig().lightsStyle);
                light->ColorWasSet(color);
            }

            return;
        }
        else if (TechnicolourController::randFloat() < getConfig().lightsFrequency)
        {
            UnityEngine::Color color = TechnicolourController::getTechnicolour(warm, beatmapEventData->time, getConfig().lightsStyle);
            switch (getConfig().lightsGrouping)
            {
                case TechnicolourLightsGrouping::ISOLATED_GROUP:
                    Chroma::LightAPI::setLightColorSafe(self->event, false, Chroma::LightAPI::LSEData{color, color, color, color});
                    break;

                case TechnicolourLightsGrouping::STANDARD:
                default:
                    Chroma::LightAPI::setAllLightingColorsSafe(false, Chroma::LightAPI::LSEData{color, color, color, color});
                    break;
            }
        }
    }

    LightSwitchEventEffect_HandleBeatmapObjectCallbackControllerBeatmapEventDidTrigger(self, beatmapEventData);
}

MAKE_HOOK_MATCH(BloomPrePassBackgroundColorsGradientFromColorSchemeColors_Start, &BloomPrePassBackgroundColorsGradientFromColorSchemeColors::Start, void, BloomPrePassBackgroundColorsGradientFromColorSchemeColors* self) {
    BloomPrePassBackgroundColorsGradientFromColorSchemeColors_Start(self);
    if (getConfig().disableGradientBackground)
    {
        BloomPrePassBackgroundColorsGradient* gradient = currentBloomColours->bloomPrePassBackgroundColorsGradient;
        gradient->elements->values[0]->color = Sombrero::FastColor::get_black();
        gradient->elements->values[1]->color = Sombrero::FastColor::get_black();
        gradient->elements->values[5]->color = Sombrero::FastColor::get_black();
        gradient->UpdateGradientTexture();
    }   else    {
        currentBloomColours = self;
    }
}

void installHooks() {
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), BaseNoteVisuals_HandleNoteControllerDidInit);
    INSTALL_HOOK(getLogger(), ObstacleController_Init);
    INSTALL_HOOK(getLogger(), LightSwitchEventEffect_HandleBeatmapObjectCallbackControllerBeatmapEventDidTrigger);
    INSTALL_HOOK(getLogger(), BeatmapObjectCallbackController_Start);
    INSTALL_HOOK(getLogger(), BloomPrePassBackgroundColorsGradientFromColorSchemeColors_Start);
    getLogger().info("Installed all hooks!");
}