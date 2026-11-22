#pragma once

static Start TamoraOffer(const char* npc, const char* dialog,
                         std::initializer_list<std::pair<std::optional<int>, int>> checks = {}) {
    const auto ready = Require(checks);
    if (ready != Start::Yes) return ready;
    // Having used an offer does not by itself prove that its quest was taken or lost.
    return Told(dialog) == 0 ? WithNpc(Start::Yes, npc) : Start::Unknown;
}

static Start ReadableStart(const char* item, const char* readFlag) {
    return HasUnpackedItem(item) && Value(readFlag) == 0 ? Start::Yes : Start::Unknown;
}

static Start TamoraStartRule(OQITamora::QuestId id) {
    namespace T = OQITamora;
    const auto chapter = Value("KAPITEL");
    if (!chapter) return Start::Unknown;
    if (*chapter < 2) return Start::Later;
    if (*chapter != 2) return Start::Unknown;

    const auto mission = Value("MIS_TAMORAMISSION");
    switch (id) {
    case T::Mission:
        return TamoraOffer("PAL_99002_GERNOT", "DIA_GERNOT_TAMORA",
            {{Told("DIA_GERNOT_KONDEHRAMISSONBEENDET"), 1}, {Value("MIS_KONDEHRAMISSION"), 1},
             {Value("MIS_WOLFGANG_DIEBSTAHL"), 2}, {Value("MIS_SVEN_SUMPFKRAUTHANDEL"), 2}});
    case T::Blade: {
        const auto start = TamoraOffer("PAL_99002_GERNOT", "DIA_GERNOT_PALADINKLINGE",
            {{Told("DIA_GERNOT_TAMORA"), 1}, {Value("MIS_FLUCHFRIEDHOF"), 1},
             {Told("DIA_STAMMLER_RITUAL"), 1}});
        return start == Start::Yes ? start : Start::Unknown; // A later Faring trigger is also possible.
    }
    case T::Fleet: return StartRule(Fleet);
    case T::Goblins: return StartRule(Goblins);
    case T::Chronicles: {
        static constexpr const char* books[] = {
            "ITWR_TAMORACHRONIK1", "ITWR_TAMORACHRONIK2", "ITWR_TAMORACHRONIK3",
            "ITWR_TAMORACHRONIK4", "ITWR_TAMORACHRONIK5"
        };
        static constexpr const char* read[] = {
            "TAMORACHRONIK1", "TAMORACHRONIK2", "TAMORACHRONIK3", "TAMORACHRONIK4", "TAMORACHRONIK5"
        };
        for (std::size_t i = 0; i < std::size(books); ++i) {
            if (ReadableStart(books[i], read[i]) == Start::Yes) return Start::Yes;
        }
        return Start::Unknown;
    }
    case T::Tower:
        return mission && *mission != 2 ? ReadableStart("ITWR_BRIEFVERSUNKENERTURM", "BRIEFVERSUNKENERTURMGELESEN")
                                        : Start::Unknown;
    case T::Swamp: return ReadableStart("ITWR_DANIELBRIEF", "DANIELSBRIEFGELESEN");
    case T::Pickpocket: return Start::Unknown;
    default: break;
    }

    if (!Value("CURRENTLEVEL")) return Start::Unknown;
    if (!OnTamora()) return Start::Later;

    constexpr const char* bruno = "PAL_3_BRUNO";
    constexpr const char* stephan = "PAL_21_STEPHAN";
    constexpr const char* servant = "NONE_22008_DIENER";
    constexpr const char* barkeeper = "VLK_56_BARKEEPER";
    constexpr const char* hermit = "NONE_22010_EINSIEDLER";
    switch (id) {
    case T::Governor:
        return TamoraOffer("MIL_29_JOSEPH", "MIL_29_JOSEPH_GERNOT",
            {{Told("MIL_29_JOSEPH_ALBATROS"), 1}});
    case T::Advisor:
    case T::Bribe:
        return TamoraOffer(stephan, "DIA_PAL_21_STEPHAN_WASTUN",
            {{Told("DIA_PAL_21_STEPHAN_HALLO"), 1}, {Value("MIS_WEGZUBRUNO"), 1}});
    case T::Arrows:
        return TamoraOffer("MIL_14_FINKO", "MIL_14_FINKO_LASSMICHRATEN",
            {{Told("MIL_14_FINKO_ERSTEBEGEGNUNG"), 1}});
    case T::Berries:
        return TamoraOffer(barkeeper, "VLK_56_BARKEEPER_SCHNAPSBEEREN",
            {{Told("DIA_VLK_56_BARKEEPER_HALLO"), 1}});
    case T::Juniper:
        return TamoraOffer(barkeeper, "VLK_56_BARKEEPER_BESONDERERSCHNAPS",
            {{Value("MIS_SCHNAPSBEEREN"), 2}, {mission, 1}});
    case T::Dice:
        return TamoraOffer("MIL_12512_MITCH", "DIA_12512_MITCH_LANGEWEILE",
            {{Told("DIA_12512_MITCH_HALLO"), 1}, {Value("ENTERTAMORA_KAPITEL4"), 0}});
    case T::Honey:
        return TamoraOffer("VLK_7701_BUERGERIN", "DIA_7701_BUERGERIN_HILFE",
            {{Told("DIA_7701_BUERGERIN_FULKO"), 1}, {Told("DIA_7701_BUERGERIN_BEWOHNER"), 1},
             {Told("DIA_7701_BUERGERIN_STATTHALTER"), 1}, {mission, 1}});
    case T::Watch: {
        if (ReadableStart("ITWR_MOD_TURMWACHEZETTEL", "TURMWACHELETZTENACHRICHTGELESEN") == Start::Yes) {
            return Start::Yes;
        }
        const auto start = TamoraOffer(stephan, "DIA_PAL_21_STEPHAN_WACHTUM",
            {{Told("DIA_PAL_21_STEPHAN_WASTUN"), 1}, {Value("TURMWACHELETZTENACHRICHTGELESEN"), 0}});
        return start == Start::Yes ? start : Start::Unknown;
    }
    case T::Thief:
        return TamoraOffer("MIL_31_KLAUS", "DIA_KLAUS_FINDE_DEN_DIEB",
            {{Told("DIA_KLAUSTAMORA_HALLO"), 1}});
    case T::Stammler: {
        const auto start = TamoraOffer(barkeeper, "DIA_VLK_56_BARKEEPER_INFOGEGEND",
            {{Told("DIA_VLK_56_BARKEEPER_HALLO"), 1}});
        return start == Start::Yes ? start : Start::Unknown; // There are also crypt and book starts.
    }
    case T::Bow:
        return TamoraOffer("OUT_99_HANS", "OUT_99_HANS_BOGENHOLEN",
            {{Told("OUT_99_HANS_VOMWAFFENHAENDLER"), 1}});
    case T::Hides:
        return TamoraOffer("OUT_10000_MARKUS", "OUT_10000_MARKUS_VONFINKO",
            {{Told("OUT_10000_MARKUS_HALLO"), 1}, {Told("OUT_99_HANS_DEINPAKET"), 1}});
    case T::Prisoner: {
        const auto start = TamoraOffer(barkeeper, "VLK_56_BARKEEPER_INFOFULKO",
            {{Told("VLK_56_BARKEEPER_SCHNAPSBEEREN"), 1}, {Value("MIS_FULKO"), 1},
             {Told("DIA_FULKO_ORKFLOTTE"), 1}});
        return start == Start::Yes && FindLivingNpc("NONE_22007_FULKO") ? Start::Yes : Start::Unknown;
    }
    case T::Servant: {
        const auto start = TamoraOffer(servant, "DIA_DIENERTAMORA_HERKUNFT",
            {{Told("DIA_DIENERTAMORA_ORKFLOTTE"), 1}});
        return start == Start::Yes ? start : Start::Unknown;
    }
    case T::Ghosts:
    case T::Hall:
        return TamoraOffer(servant, "DIA_DIENERTAMORA_ESSPUKT",
            {{Told("DIA_DIENERTAMORA_AMULETTGEFUNDEN"), 1}, {Value("MIS_FULKO"), 1},
             {Value("MIS_TAGEBUCHDESDIENERS"), 2}, {Value("MIS_DERDIEB"), 2},
             {Value("TAGEBUCHDESDIENERSGELESEN"), 1}});
    case T::Poltergeist:
        return TamoraOffer(servant, "DIA_DIENERTAMORA_GEISTERSTUNDE",
            {{Value("MIS_GEISTERSTUNDESAAL"), 1}, {Value("GEISTERSTUNDETAMORA"), 1}});
    case T::Negotiations: {
        const auto start = TamoraOffer(bruno, "DIA_PAL_3_BRUNO_VERHANDLUNGEN",
            {{Told("DIA_PAL_3_BRUNO_HALLO"), 1}, {Value("MIS_FULKO"), 2}, {Value("VERHANDLUNGENJACKO"), 0}});
        const auto mercenaries = Value("MIS_ORKSOELDNERTAMORA");
        if (start == Start::Yes && mercenaries && *mercenaries != 2) {
            // Fulko's death is a prerequisite of the report to Bruno.
            if (Told("DIA_PAL_3_BRUNO_FULKOTOT") == 1 && FindLivingNpc("DMT_4286_JACKO")) return Start::Yes;
        }
        return start == Start::Later ? Start::Later : Start::Unknown;
    }
    case T::Mines:
    case T::Miners:
        return TamoraOffer(bruno, "DIA_PAL_3_BRUNO_MINEN",
            {{Told("DIA_PAL_3_BRUNO_FUERDENKOENIG"), 1}, {mission, 1}});
    case T::Salt:
        return TamoraOffer(bruno, "DIA_PAL_3_BRUNO_SALZMINE", {{Value("MIS_TAMORAMINEN"), 1}});
    case T::Ore:
        return TamoraOffer(bruno, "DIA_PAL_3_BRUNO_ERZMINE", {{Value("MIS_TAMORAMINEN"), 1}});
    case T::Silver: {
        const auto start = TamoraOffer(bruno, "DIA_PAL_3_BRUNO_SILBERMINE", {{Value("MIS_TAMORAMINEN"), 1}});
        return start == Start::Yes ? start : Start::Unknown;
    }
    case T::Adanos:
        return TamoraOffer("KDW_18_TELEM", "DIA_TELEM_ADANOSSCHREIN",
            {{Value("KLAUSFINDEDENDIEBMEINETWEGEN"), 1}, {Value("MIS_DERDIEB"), 2}});
    case T::Dress:
        return TamoraOffer("VLK_7710_BUERGERIN", "DIA_7710_BUERGERIN_HILFE",
            {{Told("DIA_7710_BUERGERIN_STATTHALTER"), 1}});
    case T::Griffin:
        return TamoraOffer("NONE_22009_SCHAEFER", "DIA_22009_SCHAEFER_PROBLEM",
            {{Told("DIA_NONE_22009_SCHAEFER_PEOPLE"), 1}});
    case T::Treasure: {
        const auto start = TamoraOffer(hermit, "DIA_22010_EINSIEDLER_SCHATZSUCHE",
            {{Told("DIA_22010_EINSIEDLER_NOCHMALWARUMHIER"), 1}});
        return start == Start::Yes ? start : Start::Unknown;
    }
    case T::Book:
        return TamoraOffer("PAL_13_RUBEN", "DIA_RUBEN_DANIELSSUCHE",
            {{Told("DIA_RUBEN_DANIELSBRIEF"), 1}, {Value("DANIELSBRIEFGELESEN"), 1},
             {Value("MIS_VERMISSTERBRUDER"), 1}});
    default: return Start::Unknown;
    }
}

static bool TamoraClosedRule(OQITamora::QuestId id, const Facts& facts) {
    namespace T = OQITamora;
    switch (id) {
    case T::Dice:
    case T::Berries:
    case T::Miners:
        return facts.flag == 3;
    case T::Negotiations:
        return facts.flag == 3 || (facts.flag == 0 && Value("MIS_ORKSOELDNERTAMORA") == 2);
    case T::Griffin:
        return facts.flag == 0 && Value("KEINEJAGDAUFDENGREIF") == 1
            && Told("DIA_22009_SCHAEFER_PROBLEM") == 1;
    case T::Juniper:
    case T::Honey:
    case T::Tower:
        // Establish that the quest never became active before the main quest was turned in.
        return facts.flag == 0 && facts.journal == Journal::Absent && Value("MIS_TAMORAMISSION") == 2;
    default:
        return false;
    }
}
