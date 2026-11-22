#pragma once

using OQIDeparture::Step;

static std::optional<bool> IsValue(const char* name, int expected) {
    const auto value = Value(name);
    if (!value) return std::nullopt;
    return *value == expected;
}

static bool NpcsAvailable(std::initializer_list<const char*> names) {
    return std::all_of(names.begin(), names.end(), [](const char* name) { return static_cast<bool>(FindLivingNpc(name)); });
}

static bool HasUntakenQuest(const Facts& facts) {
    return facts.flag == 0 && facts.journal == Journal::Absent;
}

static Step KondehraDeparture(QuestId id, const Facts& facts, Status status) {
    using namespace OQIDeparture;
    constexpr const char* sandrock = "MIL_8101_SANDROCK";
    constexpr const char* gyges = "BAU_8102_GYGES";
    constexpr const char* olderson = "BAU_8104_OLDERSON";
    constexpr const char* oskar = "BAU_8103_OSKAR";
    const auto wreck = IsValue("MIS_GESUNKENESSCHIFF", 2);
    switch (id) {
    case Island:
        // The journal closes aboard ship. Examining the wreck completes the work ashore.
        if (status == Status::Done) return Step::Done;
        return Checkpoint(wreck, facts.flag == 1 && NpcsAvailable({olderson, sandrock, oskar, gyges}));
    case Fleet:
        // The relevant evidence ashore is already tracked by the wreck and main mission.
        return Step::Deferred;
    case Goblins: {
        if (status == Status::Done) return Step::Done;
        const auto treasureNotes = IsValue("PIRATENGOBLINSBUCHGELESEN", 1);
        const auto raftNotes = IsValue("PIRATENGOBLINSBUCH2GELESEN", 1);
        if (!treasureNotes || !raftNotes) return Step::Unknown;
        return Checkpoint(*treasureNotes && *raftNotes,
                          status != Status::Done && status != Status::Missed);
    }
    case Ring:
        if (status == Status::Done) return Step::Done;
        return Checkpoint(IsValue("READ_OLDSTORY1", 1), status != Status::Done && status != Status::Missed);
    case Testament:
        if (status == Status::Done) return Step::Done;
        if (status == Status::Missed) return Step::Missed;
        return Checkpoint(IsValue("KOROFSKITAGEBUCH1GELESEN", 1), status != Status::Done && NpcsAvailable({sandrock}));
    case Shrine:
        return Whole(status, NpcsAvailable({oskar, gyges}));
    case Couple:
        return Whole(status, NpcsAvailable({"BAU_8107_HEIDELINDE", "BAU_8105_BROSIUS"}));
    case Wolf:
        return Whole(status, NpcsAvailable({oskar, gyges}));
    case Errand:
        return Whole(status, NpcsAvailable({oskar, "BAU_8106_GRISELDA"}));
    case Fortress:
    case Library:
    case Rats:
        return Whole(status, NpcsAvailable({sandrock}));
    case Wreck:
        return Whole(status, NpcsAvailable({olderson, sandrock, oskar, gyges}));
    case Staff:
        return Whole(status, NpcsAvailable({oskar, gyges}));
    case Provisions: {
        // The script accepts two types of wine. Only a sufficient inventory
        // observed on the current visit proves everything can be turned in.
        const bool supplies = UnpackedAmount("ITFO_BACON") >= 10
            && UnpackedAmount("ITFO_TOMATE") >= 10
            && UnpackedAmount("ITFO_CHEESE") >= 10
            && UnpackedAmount("ITMI_MEHLSACK") >= 5
            && (HasUnpackedItem("ITMI_WEINFASS") || HasUnpackedItem("ITMI_WEINFASS_VARANT"));
        return Whole(status, supplies && NpcsAvailable({sandrock}));
    }
    default: return Step::Unknown;
    }
}

static Step TamoraDeparture(OQITamora::QuestId id, const Facts& facts, Status status) {
    namespace T = OQITamora;
    using namespace OQIDeparture;
    constexpr const char* bruno = "PAL_3_BRUNO";
    constexpr const char* stephan = "PAL_21_STEPHAN";
    constexpr const char* servant = "NONE_22008_DIENER";
    constexpr const char* barkeeper = "VLK_56_BARKEEPER";
    constexpr const char* telem = "KDW_18_TELEM";
    constexpr const char* hermit = "NONE_22010_EINSIEDLER";
    const auto fulko = Value("MIS_FULKO");
    const auto mission = Value("MIS_TAMORAMISSION");
    switch (id) {
    case T::Mission: {
        if (status == Status::Done) return Step::Done;
        const auto objectives = Require({
            {Value("MIS_WEGZUBRUNO"), 2}, {fulko, 2}, {Value("MIS_ORKSOELDNERTAMORA"), 2},
            {Value("MIS_TAMORAMINEN"), 2}, {Value("MIS_FLUCHFRIEDHOF"), 2}
        });
        if (objectives == Start::Yes) return Step::Done;
        return Whole(status, objectives == Start::Later && NpcsAvailable({bruno, stephan, barkeeper}));
    }
    case T::Blade:
    case T::Miners:
    case T::Book:
        return Step::Deferred;
    case T::Cellar:
        return Checkpoint(IsValue("KELLERVONTAMORAENTDECKT", 1), status != Status::Done && NpcsAvailable({bruno}));
    case T::Fleet: {
        if (status == Status::Done) return Step::Done;
        const auto told = Told("DIA_DIENERTAMORA_ORKFLOTTE");
        if (!told) return Step::Unknown;
        return Checkpoint(*told == 1, mission == 1 && NpcsAvailable({servant, bruno}));
    }
    case T::Goblins:
        if (status == Status::Done) return Step::Done;
        return Checkpoint(IsValue("PIRATENFLOSS_TAMORA_ENTDECKT", 1), status != Status::Done);
    case T::Pickpocket:
        if (status == Status::Done) return Step::Done;
        if (facts.flag == 0) return Step::Deferred; // The quest is offered by Sven aboard ship.
        return Checkpoint(IsValue("TALENT_SCHLOESSERKNACKEN2", 1), NpcsAvailable({"NONE_4295_DIEB"}));
    case T::Mercenaries:
        return Whole(status, NpcsAvailable({bruno, stephan}), HasUntakenQuest(facts));
    case T::Governor:
        return Whole(status, NpcsAvailable({"MIL_29_JOSEPH", bruno, stephan}));
    case T::Advisor:
    case T::Bribe:
        return Whole(status, NpcsAvailable({bruno, stephan}));
    case T::Arrows:
        return Whole(status, NpcsAvailable({"MIL_14_FINKO", "OUT_99_HANS", "OUT_10000_MARKUS"}));
    case T::Berries:
        return Whole(status, NpcsAvailable({barkeeper}));
    case T::Juniper: {
        const auto berries = Value("MIS_SCHNAPSBEEREN");
        return Whole(status, mission == 1 && berries && (*berries == 0 || *berries == 1 || *berries == 2)
                     && NpcsAvailable({barkeeper}));
    }
    case T::Dice:
        return Whole(status, Value("ENTERTAMORA_KAPITEL4") == 0 && NpcsAvailable({"MIL_12512_MITCH"}));
    case T::Honey:
        return Whole(status, mission == 1 && NpcsAvailable({"VLK_7701_BUERGERIN"}));
    case T::Watch:
        return Whole(status, NpcsAvailable({stephan}), HasUntakenQuest(facts));
    case T::Thief:
        return Whole(status, NpcsAvailable({"MIL_31_KLAUS", "NONE_4295_DIEB", telem}));
    case T::Stammler:
        return Whole(status, NpcsAvailable({barkeeper, bruno}), HasUntakenQuest(facts));
    case T::Chronicles:
        // All five tomes are in TAMORA.ZEN. Some are behind local quest branches.
        return Whole(status, NpcsAvailable({bruno, barkeeper, stephan}), HasUntakenQuest(facts));
    case T::Bow:
        return Whole(status, NpcsAvailable({"OUT_99_HANS"}));
    case T::Hides:
        return Whole(status, NpcsAvailable({"OUT_10000_MARKUS", "OUT_99_HANS"}));
    case T::Prisoner:
        return Whole(status, NpcsAvailable({"OUT_22008_VICTOR", servant}), HasUntakenQuest(facts));
    case T::Servant:
        return Whole(status, NpcsAvailable({servant}), HasUntakenQuest(facts));
    case T::Ghosts:
    case T::Hall:
    case T::Poltergeist:
        return Whole(status, fulko && (*fulko == 0 || *fulko == 1) && NpcsAvailable({servant}));
    case T::Negotiations:
        return Whole(status, NpcsAvailable({bruno, "DMT_4286_JACKO"}));
    case T::Mines:
    case T::Salt:
    case T::Ore:
    case T::Silver:
        return Whole(status, NpcsAvailable({bruno}), id == T::Silver && HasUntakenQuest(facts));
    case T::Tower:
        return Whole(status, mission == 1, HasUntakenQuest(facts));
    case T::Adanos:
        return Whole(status, NpcsAvailable({telem, "MIL_31_KLAUS", "NONE_4295_DIEB"}));
    case T::Dress:
        return Whole(status, NpcsAvailable({"VLK_7710_BUERGERIN"}));
    case T::Griffin:
        return Whole(status, NpcsAvailable({"NONE_22009_SCHAEFER"}));
    case T::House:
        return Whole(status, NpcsAvailable({hermit}), HasUntakenQuest(facts));
    case T::Treasure:
        return Whole(status, NpcsAvailable({hermit}), HasUntakenQuest(facts));
    case T::Swamp:
        return Whole(status, NpcsAvailable({"PAL_13_RUBEN"}), HasUntakenQuest(facts));
    default: return Step::Unknown;
    }
}

static Step AlbatrosDeparture(OQIAlbatros::QuestId id, const Facts& facts, Status status) {
    namespace A = OQIAlbatros;
    using namespace OQIDeparture;
    const auto level = Value("CURRENTLEVEL");
    const bool firstAnchorage = level == Value("SEAWORLD_ZEN");
    const bool merchant = level == Value("SEAWORLD3_ZEN");
    const bool battle = level == Value("SEAWORLDFIGHT_ZEN");
    constexpr const char* harald = "MIL_9000_HARALD";
    constexpr const char* halvar = "MIL_9001_HALVAR";
    constexpr const char* wolfgang = "MIL_9002_WOLFGANG";
    constexpr const char* gernot = "PAL_99002_GERNOT";
    constexpr const char* albin = "MIL_9004_ALBIN";
    constexpr const char* dandolo = "MIL_9005_DANDOLO";
    const bool wreckAccess = firstAnchorage || HasUnpackedItem("ITWR_SEAMAP_RAUHBEINSSCHATZKARTE")
        || Value("MIS_DANDOLO_STRANDGUT") == 2;

    switch (id) {
    case A::Lookout:
        if (battle) return Whole(status, NpcsAvailable({gernot}));
        if (Value("MIS_KHORUSPROVIANT") == 2 && status == Status::Available) {
            // Taking the watch launches the sea scene where it can be reported.
            return Step::Open;
        }
        return Step::Deferred;
    case A::Destination:
        if (Told("DIA_BALDWIN_ORKANGRIFFKHORINIS") == 1) {
            return Whole(status, NpcsAvailable({"PAL_99000_BALDWIN"}), HasUntakenQuest(facts));
        }
        return Step::Deferred;
    case A::Merchant:
    case A::Hold:
        return merchant ? Whole(status, NpcsAvailable({harald, "DJG_82001_AHAB"}), HasUntakenQuest(facts))
                        : Step::Deferred;
    case A::LowerDeck:
        return level == Value("SEAWORLD2_ZEN") ? Whole(status, true) : Step::Deferred;
    case A::PirateGold:
        if (!firstAnchorage && !HasUnpackedItem("ITWR_SEAMAP_RAUHBEINSSCHATZKARTE")) return Step::Deferred;
        return Checkpoint(IsValue("JONESSCHATZKARTEGEFUNDEN", 1), wreckAccess && status != Status::Done);
    case A::Boatswain:
        return Whole(status, wreckAccess && NpcsAvailable({harald, halvar, wolfgang, gernot, albin}));
    case A::Armor:
        return Whole(status, UnpackedAmount("ITMI_GOLD") >= 2000 && NpcsAvailable({harald}));
    case A::Respect:
    case A::Cook:
        return Whole(status, (wreckAccess || Value("MIS_HARALD_DECKFEGEN") == 2)
                     && NpcsAvailable({harald, wolfgang, albin}));
    case A::Duty:
    case A::Flotsam:
        return Whole(status, wreckAccess && NpcsAvailable({harald, dandolo}));
    case A::Fish:
        return Whole(status, (UnpackedAmount("ITFO_FISH") >= 24 || Told("DIA_ALBIN_FISCHEERFOLG") == 1)
                     && NpcsAvailable({albin}));
    case A::Stores:
        return Whole(status, NpcsAvailable({wolfgang}));
    case A::Contest:
        return Whole(status, NpcsAvailable({halvar, wolfgang}));
    case A::Sextant:
        return Whole(status, (firstAnchorage || HasUnpackedItem("ITMI_SEXTANT"))
                     && NpcsAvailable({halvar, gernot}));
    case A::Bully:
        return Whole(status, NpcsAvailable({"MIL_9007_ERNST", "MIL_9006_KNUT"}));
    case A::Trade:
        return Whole(status, NpcsAvailable({wolfgang, "MIL_9003_SVEN"}), HasUntakenQuest(facts));
    case A::Light:
        return Whole(status, HasUnpackedItem("ITSC_PALLIGHT") && NpcsAvailable({"MIL_9007_ERNST"}));
    case A::Herbs:
        return Whole(status, UnpackedAmount("ITPL_MANA_HERB_01") >= 15 && UnpackedAmount("ITPL_HEALTH_HERB_01") >= 20
                     && NpcsAvailable({"KDF_9999_WULFIAS"}));
    case A::Pay:
        return Whole(status, NpcsAvailable({harald, gernot}));
    case A::OreWeapon:
        return Whole(status, UnpackedAmount("ITMI_NUGGET") >= 5 && NpcsAvailable({"MIL_9008_HAWK"}));
    case A::Orcs:
        if (status == Status::Done) return Step::Done;
        return battle ? Whole(status, NpcsAvailable({"MIL_900801_HAWK"}), HasUntakenQuest(facts)) : Step::Deferred;
    case A::Pickpocket:
        if (status == Status::Done) return Step::Done;
        return Whole(status, Value("TALENT_SCHLOESSERKNACKEN2") == 1 || NpcsAvailable({"MIL_9003_SVEN"}));
    default: return Step::Unknown;
    }
}

static Step DepartureStep(std::size_t index, const Facts& facts, Status status) {
    switch (SelectedLocation()) {
    case Location::Kondehra: return KondehraDeparture(static_cast<QuestId>(index), facts, status);
    case Location::Tamora: return TamoraDeparture(static_cast<OQITamora::QuestId>(index), facts, status);
    case Location::Albatros: return AlbatrosDeparture(static_cast<OQIAlbatros::QuestId>(index), facts, status);
    }
    return Step::Unknown;
}
