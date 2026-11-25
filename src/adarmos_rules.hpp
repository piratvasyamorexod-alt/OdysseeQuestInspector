#pragma once

namespace OQIAdarmos {

inline constexpr const char* elder = "VLK_6006_VATER";
inline constexpr const char* officer = "BDT_7970_KORSARENOFFIZIER";
inline constexpr const char* mount = "BDT_7979_MOUNT";
inline constexpr const char* hunter = "BDT_7980_PLUNDERERDERKORSAREN";
inline constexpr const char* bjorn = "BDT_7981_BJORNDERWILDE";
inline constexpr const char* emilio = "BDT_6032_EMILIO";
inline constexpr const char* kosta = "SLD_6022_KOSTA";
inline constexpr const char* rosh = "SLD_6034_ROSH";
inline constexpr const char* alvares = "NONE_6026_ALVARES";
inline constexpr const char* dale = "SLD_6019_DALE";
inline constexpr const char* erik = "BDT_7976_KORSAR";
inline constexpr const char* bantor = "VLK_5031_BANTOR";

}  // namespace OQIAdarmos

static Start AdarmosStartRule(OQIAdarmos::QuestId id) {
    namespace A = OQIAdarmos;
    const auto chapter = Value("KAPITEL");
    if (!chapter || *chapter > 2) return Start::Unknown;
    if (*chapter < 2) return Start::Later;

    if (id == A::Mission) {
        return DialogOffer("PAL_99002_GERNOT", "DIA_GERNOT_ADARMOS",
            {{Told("DIA_GERNOT_SEXTANTGEBEN"), 1}, {Value("MIS_WOLFGANG_DIEBSTAHL"), 2},
             {Value("MIS_SVEN_SUMPFKRAUTHANDEL"), 2}, {Value("MIS_KONDEHRAMISSION"), 2},
             {Value("MIS_TAMORAMISSION"), 2}});
    }
    if (!Value("CURRENTLEVEL")) return Start::Unknown;
    if (!OnAdarmos()) return Start::Later;

    switch (id) {
    case A::Corsairs:
        return DialogOffer(A::officer, "DIA_OFFIZIER_HI");
    case A::Smith:
        return DialogOffer(A::elder, "DIA_DAD_SCHMIED", {{Told("DIA_DAD_PROBLEMMITSOELDNERN"), 1}});
    case A::Rosh:
        return DialogOffer(A::elder, "DIA_DAD_WHATHAPPENSNOW", {{Told("DIA_DAD_FIN"), 1}});
    case A::LazyWorker:
        return DialogOffer(A::bantor, "DIA_5031_BANTOR_INFORMATION", {{Told("DIA_5031_BANTOR_HALLO"), 1}});
    case A::Dice:
        return DialogOffer(A::dale, "DIA_DALE_PISSEINS",
            {{NpcNearWaypoint(A::dale, "INSEL_BAUERNHAUS_168", 1000.0f), 1}});
    case A::Scout:
        return DialogOffer(A::mount, "DIA_ADDON_MOUNT_FINDGUY", {{Told("DIA_ADDON_MOUNT_JOB"), 1}});
    case A::Meat: {
        const auto fromMount = DialogOffer(A::mount, "DIA_ADDON_MOUNT_NOCHWAS",
            {{Told("DIA_ADDON_MOUNT_GOOD"), 1}, {Value("HUNTING"), 0}});
        const auto fromHunter = DialogOffer(A::hunter, "DIA_ADDON_PLUNDERERDERKORSAREN_HUNT",
            {{Told("DIA_ADDON_PLUNDERERDERKORSAREN_NEWS"), 1}, {Value("HUNTING"), 0}});
        if (fromMount == Start::Yes || fromHunter == Start::Yes) return Start::Yes;
        return fromMount == Start::Later && fromHunter == Start::Later ? Start::Later : Start::Unknown;
    }
    case A::RoshIntel: {
        const auto lostFight = Value("FIGHT_LOST");
        if (!lostFight) return Start::Unknown;
        return DialogOffer(A::alvares, "DIA_ADDON_AL_MAUL_GAVE",
            {{Told("DIA_ADDON_AL_MAUL"), 1}, {NpcFirstAiVar(A::alvares), *lostFight}});
    }
    case A::Amulet:
        return DialogOffer(A::erik, "DIA_ERIK_HOL", {{Told("DIA_ERIK_NERV"), 1}});
    case A::Herbs:
        return DialogOffer(A::bjorn, "DIA_BJORN_KRAEUTERHOL", {{Told("DIA_OFFIZIER_HI"), 1}});
    case A::VillageGold:
        return DialogOffer(A::officer, "DIA_OFFIZIER_YOURQUESTFORME",
            {{Told("DIA_OFFIZIER_HI"), 1}, {Told("DIA_OFFIZIER_ALL"), 1}});
    case A::Mage:
        return DialogOffer(A::bjorn, "DIA_BJORN_AQUA", {{Told("DIA_BJORN_ROSH"), 1}});
    case A::Note:
        return ReadableStart("ITWR_LETTERTOEMILIOFOUND", "EMILIOSZETTELGELESEN");
    case A::Emilio:
        if (InventoryAmount("ITWR_LETTERTOEMILIOFOUND") == 0) return Start::Later;
        return DialogOffer(A::emilio, "DIA_EMILIO_LETTERA", {{Told("DIA_EMILIO_HEY"), 1}});
    case A::LostSon:
        return DialogOffer(A::elder, "DIA_DAD_SOHN", {{Told("DIA_DAD_GOLD"), 1}});
    default:
        return Start::Unknown;
    }
}

static OQIKondehra::Result InspectAdarmos(OQIAdarmos::QuestId id, OQIKondehra::Facts& facts) {
    namespace A = OQIAdarmos;
    using namespace OQIKondehra;
    if (id == A::Rosh && facts.flag == 0 && Told("DIA_KOSTA_MELDETWOOFTWO") == 1) {
        // The completion dialogue records success while assigning LOG_MISSION (0).
        facts.flag = 2;
    }
    facts.windowClosed = (id == A::Herbs && facts.flag == 3)
        || (id == A::Emilio && facts.flag == 0 && Told("DIA_EMILIO_LETTERA") == 1);
    if (facts.flag == 0) facts.start = AdarmosStartRule(id);
    return Evaluate(facts);
}
