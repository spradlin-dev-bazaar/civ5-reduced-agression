#include "CvGameCoreDLLPCH.h"
#include "CvDiplomacyAI.h"

#include "CivHooks.h"
#include "CivPluginApi.h"

#include <stdarg.h>
#include <stdio.h>

#define PLUGIN_EXPORT extern "C" __declspec(dllexport)

CIV_PLUGIN_EXPORT_API_VERSION;

static const CivHostInfo* s_host = 0;

// Writes a line to Logs\Bazaar.log, headed with this plugin's name.
static void Log(const char* format, ...)
{
	char line[512];
	va_list args;
	va_start(args, format);
	_vsnprintf_s(line, sizeof(line), _TRUNCATE, format, args);
	va_end(args);
	s_host->Log(s_host->pluginIndex, line);
}

static void Check(int result, const char* hook)
{
	if (result != CIV_HOOK_OK)
		Log("could not register %s (result %d)", hook, result);
}

static void Hook(int hookId, int kind, void* hook, const char* name)
{
	Check(s_host->RegisterHook(s_host->pluginIndex, hookId, kind, hook), name);
}

static PlayerTypes s_rater = NO_PLAYER;
static int s_humanStrengthPercent = 100;

static int s_cancelPercent = -1;
static int s_loggedPercent[MAX_PLAYERS];

static int CancelPercent()
{
	if (s_cancelPercent < 0)
	{
		s_cancelPercent = 100;
		Database::Results results;
		if (GC.GetGameDatabase()->Execute(results, "SELECT Value FROM Defines WHERE Name = 'REDUCED_AGGRESSION_CANCEL_PERCENT'") && results.Step())
		{
			s_cancelPercent = std::min(100, std::max(0, results.GetInt(0)));
			Log("cancelling %d%% of the AI's cheaper-unit advantage", s_cancelPercent);
		}
		else
			Log("REDUCED_AGGRESSION_CANCEL_PERCENT not found, so cancelling 100%% of the AI's cheaper-unit advantage");
	}
	return s_cancelPercent;
}

// Mirrors the computer players' discount on ordinary units in CvPlayer::getProductionNeeded(UnitTypes).
static int UnitCostEdgePercent(const CvPlayer& rater)
{
	const CvHandicapInfo* prince = GC.getHandicapInfo((HandicapTypes)GC.getInfoTypeForString("HANDICAP_PRINCE", true));
	if (prince == NULL)
		return 100;
	const CvHandicapInfo& game = GC.getGame().getHandicapInfo();
	int era = rater.GetCurrentEra();
	int gameCost = game.getAITrainPercent() * std::max(0, 100 + game.getAIPerEraModifier() * era);
	int princeCost = prince->getAITrainPercent() * std::max(0, 100 + prince->getAIPerEraModifier() * era);
	if (gameCost <= 0 || princeCost <= gameCost)
		return 100;
	return princeCost * 100 / gameCost;
}

static void BeginRating(CvDiplomacyAI* ai)
{
	CvPlayer* rater = ai->GetPlayer();
	if (rater->isHuman() || rater->IsAITeammateOfHuman() || rater->isMinorCiv() || rater->isBarbarian())
		return;
	int percent = 100 + (UnitCostEdgePercent(*rater) - 100) * CancelPercent() / 100;
	if (percent != s_loggedPercent[rater->GetID()])
	{
		s_loggedPercent[rater->GetID()] = percent;
		Log("%s rates human players in peacetime at %d%% of their real strength", rater->getCivilizationShortDescription(), percent);
	}
	if (percent > 100)
	{
		s_rater = rater->GetID();
		s_humanStrengthPercent = percent;
	}
}

static void EndRating()
{
	s_rater = NO_PLAYER;
	s_humanStrengthPercent = 100;
}

static int AsRated(PlayerTypes owner, int strength)
{
	if (s_rater == NO_PLAYER)
		return strength;
	const CvPlayer& player = GET_PLAYER(owner);
	if (!player.isHuman())
		return strength;
	// Unscaled at war, or CvMilitaryAI would hold back city attacks on a target that looks stronger.
	if (GET_TEAM(GET_PLAYER(s_rater).getTeam()).isAtWar(player.getTeam()))
		return strength;
	return strength * s_humanStrengthPercent / 100;
}

static bool BeforeRatePlayer(CvDiplomacyAI* self, PlayerTypes ePlayer)
{
	BeginRating(self);
	return false;
}

static void AfterRatePlayer(CvDiplomacyAI* self, PlayerTypes ePlayer)
{
	EndRating();
}

static bool BeforeRateThreats(CvDiplomacyAI* self)
{
	BeginRating(self);
	return false;
}

static void AfterRateThreats(CvDiplomacyAI* self)
{
	EndRating();
}

static void AfterPlayerStrength(const CvPlayer* self, int& result)
{
	result = AsRated(self->GetID(), result);
}

static void AfterCityStrength(const CvCity* self, int& result)
{
	result = AsRated(self->getOwner(), result);
}

PLUGIN_EXPORT bool CivPluginEntry(const CivHostInfo* host, CivPluginInfo* plugin)
{
	s_host = host;
	plugin->name = "Reduced Aggression (Bazaar)";
	plugin->version = "1";

	Hook(CivHooks::CvDiplomacyAI_DoUpdateOnePlayerMilitaryStrength::ID, CIV_HOOK_BEFORE, (void*)&BeforeRatePlayer,
		"before CvDiplomacyAI::DoUpdateOnePlayerMilitaryStrength");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateOnePlayerMilitaryStrength::ID, CIV_HOOK_AFTER, (void*)&AfterRatePlayer,
		"after CvDiplomacyAI::DoUpdateOnePlayerMilitaryStrength");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateOnePlayerEconomicStrength::ID, CIV_HOOK_BEFORE, (void*)&BeforeRatePlayer,
		"before CvDiplomacyAI::DoUpdateOnePlayerEconomicStrength");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateOnePlayerEconomicStrength::ID, CIV_HOOK_AFTER, (void*)&AfterRatePlayer,
		"after CvDiplomacyAI::DoUpdateOnePlayerEconomicStrength");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateOnePlayerTargetValue::ID, CIV_HOOK_BEFORE, (void*)&BeforeRatePlayer,
		"before CvDiplomacyAI::DoUpdateOnePlayerTargetValue");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateOnePlayerTargetValue::ID, CIV_HOOK_AFTER, (void*)&AfterRatePlayer,
		"after CvDiplomacyAI::DoUpdateOnePlayerTargetValue");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateMilitaryThreats::ID, CIV_HOOK_BEFORE, (void*)&BeforeRateThreats,
		"before CvDiplomacyAI::DoUpdateMilitaryThreats");
	Hook(CivHooks::CvDiplomacyAI_DoUpdateMilitaryThreats::ID, CIV_HOOK_AFTER, (void*)&AfterRateThreats,
		"after CvDiplomacyAI::DoUpdateMilitaryThreats");

	Hook(CivHooks::CvPlayer_GetMilitaryMight::ID, CIV_HOOK_AFTER, (void*)&AfterPlayerStrength, "after CvPlayer::GetMilitaryMight");
	Hook(CivHooks::CvPlayer_GetEconomicMight::ID, CIV_HOOK_AFTER, (void*)&AfterPlayerStrength, "after CvPlayer::GetEconomicMight");
	Hook(CivHooks::CvPlayer_getPower::ID, CIV_HOOK_AFTER, (void*)&AfterPlayerStrength, "after CvPlayer::getPower");
	Hook(CivHooks::CvCity_GetPower::ID, CIV_HOOK_AFTER, (void*)&AfterCityStrength, "after CvCity::GetPower");
	return true;
}
