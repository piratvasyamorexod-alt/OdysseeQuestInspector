#pragma once

static Start AlbatrosStartRule(OQIAlbatros::QuestId id) {
    namespace A = OQIAlbatros;
    const auto chapter = Value("KAPITEL");
    if (!chapter) return Start::Unknown;
    if (*chapter < 2) return Start::Later;
    if (*chapter != 2) return Start::Unknown;

    if (id == A::PirateGold) {
        return ReadableStart("ITWR_SEAMAP_RAUHBEINSSCHATZKARTE", "JONESSCHATZKARTEGEFUNDEN");
    }
    if (!Value("CURRENTLEVEL")) return Start::Unknown;
    if (!OnAlbatros()) return Start::Later;

    constexpr const char* harald = "MIL_9000_HARALD";
    constexpr const char* halvar = "MIL_9001_HALVAR";
    constexpr const char* wolfgang = "MIL_9002_WOLFGANG";
    constexpr const char* sven = "MIL_9003_SVEN";
    switch (id) {
    case A::Boatswain: {
        const auto haraldOffer = DialogOffer(harald, "DIA_HARALD_JOBBOOTSMANN",
            {{Told("DIA_HARALD_HALLO"), 1}});
        const auto wolfgangOffer = DialogOffer(wolfgang, "DIA_WOLFGANG_BOOTSMANN",
            {{Told("DIA_HARALD_BOOTSMANN"), 1}});
        if (haraldOffer == Start::Yes || wolfgangOffer == Start::Yes) return Start::Yes;
        return haraldOffer == Start::Later && wolfgangOffer == Start::Later ? Start::Later : Start::Unknown;
    }
    case A::Armor:
        return DialogOffer(harald, "DIA_HARALD_RUESTUNG",
            {{Told("DIA_HARALD_BOOTSMANN"), 1}, {Told("DIA_HARALD_MATROSENKLEIDUNG"), 1}});
    case A::Respect:
        return DialogOffer(harald, "DIA_HARALD_NACHFRAGEBOOTSMANN",
            {{Told("DIA_HARALD_ENDEJOBALBIN"), 1}});
    case A::Duty:
        return DialogOffer(harald, "DIA_HARALD_WORKNOW", {{Told("DIA_HARALD_BOOTSMANN"), 1}});
    case A::Cook:
        return DialogOffer(harald, "DIA_HARALD_AUFGABEALBIN",
            {{Told("DIA_HARALD_DECKFEGENERFOLG"), 1}});
    case A::Fish:
        return DialogOffer("MIL_9004_ALBIN", "DIA_ALBIN_NEUERJOB",
            {{Told("DIA_ALBIN_PFANNEFUERALBIN"), 1}});
    case A::Stores:
        return DialogOffer(wolfgang, "DIA_WOLFGANG_DIEBSTAHL", {{Told("DIA_WOLFGANG_BOOTSMANN"), 1}});
    case A::Flotsam:
        return DialogOffer("MIL_9005_DANDOLO", "DIA_DANDOLO_STRANDGUT",
            {{Told("DIA_DANDOLO_DANDOLOSAUFTRAG"), 1}});
    case A::Contest:
        return DialogOffer(halvar, "DIA_HALVAR_WETTSAUFEN", {{Told("DIA_HALVAR_BOOTSMANN"), 1}});
    case A::Sextant:
        return DialogOffer(halvar, "DIA_HALVAR_SEXTANTSUCHE", {{Told("DIA_HALVAR_SEXTANTVERLOREN"), 1}});
    case A::Bully: {
        const auto offer = DialogOffer("MIL_9007_ERNST", "DIA_ERNST_SCHLAEGER",
            {{Told("DIA_ERNST_HALLO"), 1}});
        return offer == Start::Yes ? offer : Start::Unknown;
    }
    case A::Trade: {
        const auto offer = DialogOffer(sven, "DIA_SVEN_WELCHEWARE", {{Told("DIA_SVEN_HALLO"), 1}});
        return offer == Start::Yes ? offer : Start::Unknown;
    }
    case A::Light:
        return DialogOffer("MIL_9007_ERNST", "DIA_ERNST_LICHTZAUBERFUERERNST",
            {{Told("DIA_ERNST_MIESESTIMMUNG"), 1}});
    case A::Herbs:
        return DialogOffer("KDF_9999_WULFIAS", "DIA_WULFIAS_LERNEN", {{Told("DIA_WULFIAS_JOB"), 1}});
    case A::Pay:
        return DialogOffer(harald, "DIA_HARALD_PALADIN_GERNOT", {{Told("DIA_HARALD_PALADINE"), 1}});
    case A::OreWeapon:
        if (Value("CURRENTLEVEL") == Value("SEAWORLDFIGHT_ZEN")) return Start::Later;
        return WithNpc(Require({{ArrayValue("PLAYER_TALENT_SMITH", 0), 1}, {Value("SCHMIED_TEACHSMITH"), 0},
                                {Value("HAWK_9308_HALLO"), 1}}), "MIL_9008_HAWK");
    case A::Lookout:
        return DialogOffer("PAL_99002_GERNOT", "DIA_GERNOT_AUSGUCK", {{Value("MIS_KHORUSPROVIANT"), 2}});
    case A::Destination: {
        const auto offer = DialogOffer(harald, "DIA_HARALD_REISEWOHIN",
            {{Told("DIA_HARALD_SITUATIONFESTLAND"), 1}});
        return offer == Start::Yes ? offer : Start::Unknown;
    }
    case A::Pickpocket: {
        // The second skill test is an alternative to the three-part branch in the game.
        const auto mainBranch = Require({{Value("SVEN_STRAFEANTRETEN"), 1},
            {Value("BRAUCHETASCHENDIEBSTAHLLEHRER"), 0}, {Value("TALENT_SCHLOESSERKNACKEN2"), 0}});
        const auto alternative = Value("TALENT_SCHLOESSERKNACKEN3");
        if (mainBranch == Start::Yes || alternative == 0) return DialogOffer(sven, "DIA_SVEN_BITTE");
        return mainBranch == Start::Later && alternative == 1 ? Start::Later : Start::Unknown;
    }
    default:
        // World triggers and offers with more than one NPC variant need additional evidence.
        return Start::Unknown;
    }
}

static bool AlbatrosClosedRule(OQIAlbatros::QuestId id, const Facts& facts) {
    return id == OQIAlbatros::Bully && facts.flag == 3 && Told("DIA_KNUT_DUELL") == 1
        && Told("DIA_KNUT_AFTERFIGHT") == 1;
}
