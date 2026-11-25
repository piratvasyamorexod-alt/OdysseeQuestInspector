#include "../src/albatros.hpp"
#include "../src/departure.hpp"
#include "../src/tamora.hpp"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>

using namespace OQIKondehra;

static std::map<std::string, int> flags{{"KAPITEL", 2}, {"CURRENTLEVEL", 14}};
static std::map<std::pair<std::string, unsigned int>, int> arrayValues;
static std::map<std::string, int> dialogs;
static std::set<std::string> actors;
static std::set<std::string> items;
static std::map<std::string, int> quantities;
static std::map<std::string, int> packedQuantities;

static std::optional<int> Read(const std::map<std::string, int>& values, const char* name) {
    const auto it = values.find(name);
    if (it == values.end()) return std::nullopt;
    return it->second;
}

static std::optional<int> Value(const char* name) { return Read(flags, name); }
static std::optional<int> ArrayValue(const char* name, unsigned int index) {
    const auto it = arrayValues.find({name, index});
    return it == arrayValues.end() ? std::nullopt : std::optional<int>{it->second};
}
static std::optional<int> Told(const char* name) { return Read(dialogs, name); }
static bool OnAlbatros() { return Value("CURRENTLEVEL") == 16 || Value("CURRENTLEVEL") == 29; }
static bool OnTamora() { return Value("CURRENTLEVEL") == 14; }
static int UnpackedAmount(const char* name) {
    return Read(quantities, name).value_or(static_cast<int>(items.count(name)));
}
static bool HasUnpackedItem(const char* name) { return UnpackedAmount(name) > 0; }
static int InventoryAmount(const char* name) {
    return UnpackedAmount(name) + Read(packedQuantities, name).value_or(0);
}
static bool FindLivingNpc(const char* name) { return actors.count(name) != 0; }
static Start StartRule(QuestId) { return Start::Unknown; }

static Start Require(std::initializer_list<std::pair<std::optional<int>, int>> checks) {
    bool missing = false;
    for (const auto& [value, expected] : checks) {
        if (value && *value != expected) return Start::Later;
        if (!value) missing = true;
    }
    return missing ? Start::Unknown : Start::Yes;
}

static Start WithNpc(Start ready, const char* name) {
    if (ready != Start::Yes) return ready;
    return FindLivingNpc(name) ? Start::Yes : Start::Unknown;
}

#include "../src/tamora_rules.hpp"
#include "../src/albatros_rules.hpp"

enum class Location { Albatros, Kondehra, Tamora };
static Location chosenLocation = Location::Kondehra;
static Location SelectedLocation() { return chosenLocation; }
#include "../src/departure_rules.hpp"

static void Check(bool condition) {
    if (!condition) std::abort();
}

static Result Inspect(OQITamora::QuestId id, Facts facts) {
    facts.windowClosed = TamoraClosedRule(id, facts);
    if (facts.flag == 0) facts.start = TamoraStartRule(id);
    return Evaluate(facts);
}

static void TestTamora() {
    namespace T = OQITamora;
    Check(T::quests.size() == 40);

    Check(Inspect(T::Thief, {}).status == Status::Unknown);
    Check(Inspect(T::Thief, {0}).status == Status::Unknown);
    dialogs["DIA_KLAUSTAMORA_HALLO"] = 0;
    dialogs["DIA_KLAUS_FINDE_DEN_DIEB"] = 0;
    actors.insert("MIL_31_KLAUS");
    Check(Inspect(T::Thief, {0}).status == Status::Later);
    dialogs["DIA_KLAUSTAMORA_HALLO"] = 1;
    Check(Inspect(T::Thief, {0}).status == Status::Available);
    Check(Inspect(T::Thief, {1}).status == Status::Running);
    Check(Inspect(T::Thief, {2}).status == Status::Done);

    flags["MIS_TAMORAMISSION"] = 1;
    flags["BRIEFVERSUNKENERTURMGELESEN"] = 0;
    items.insert("ITWR_BRIEFVERSUNKENERTURM");
    Check(Inspect(T::Tower, {0}).status == Status::Available);
    flags["MIS_TAMORAMISSION"] = 2;
    Check(Inspect(T::Tower, {0}).status == Status::Missed);
    Check(Inspect(T::Tower, {0, Journal::Running}).status == Status::Running);
    Check(Inspect(T::Tower, {2}).status == Status::Done);

    Check(Inspect(T::Dice, {3}).status == Status::Missed);
    Check(Inspect(T::Bribe, {3}).status == Status::Unknown);
    Check(Inspect(T::Bribe, {2, Journal::Success}).status == Status::Done);

    actors.insert("VLK_7701_BUERGERIN");
    dialogs["DIA_7701_BUERGERIN_FULKO"] = 1;
    dialogs["DIA_7701_BUERGERIN_BEWOHNER"] = 1;
    dialogs["DIA_7701_BUERGERIN_STATTHALTER"] = 1;
    dialogs["DIA_7701_BUERGERIN_HILFE"] = 0;
    flags["MIS_TAMORAMISSION"] = 1;
    Check(Inspect(T::Honey, {0}).status == Status::Available);
    flags["CURRENTLEVEL"] = 16;
    Check(Inspect(T::Honey, {0}).status == Status::Later);
    Check(Inspect(T::Honey, {1}).status == Status::Running);
}

static Result Inspect(OQIAlbatros::QuestId id, Facts facts) {
    facts.windowClosed = AlbatrosClosedRule(id, facts);
    if (facts.flag == 0) facts.start = AlbatrosStartRule(id);
    return Evaluate(facts);
}

static void TestAlbatros() {
    namespace A = OQIAlbatros;
    flags = {{"KAPITEL", 2}, {"CURRENTLEVEL", 29}};
    dialogs.clear();
    actors.clear();
    items.clear();

    Check(A::quests.size() == 24);
    Check(Inspect(A::Boatswain, {0}).status == Status::Unknown);
    actors.insert("MIL_9000_HARALD");
    dialogs["DIA_HARALD_JOBBOOTSMANN"] = 0;
    dialogs["DIA_HARALD_HALLO"] = 0;
    dialogs["DIA_HARALD_BOOTSMANN"] = 0;
    Check(Inspect(A::Boatswain, {0}).status == Status::Later);
    dialogs["DIA_HARALD_HALLO"] = 1;
    Check(Inspect(A::Boatswain, {0}).status == Status::Available);
    Check(Inspect(A::Boatswain, {1}).status == Status::Running);
    Check(Inspect(A::Boatswain, {2}).status == Status::Done);

    actors.insert("MIL_9008_HAWK");
    flags["SEAWORLDFIGHT_ZEN"] = 8;
    flags["SCHMIED_TEACHSMITH"] = 0;
    flags["HAWK_9308_HALLO"] = 1;
    arrayValues[{"PLAYER_TALENT_SMITH", 0}] = 0;
    arrayValues[{"PLAYER_TALENT_SMITH", 1}] = 1;
    Check(Inspect(A::OreWeapon, {0}).status == Status::Later);
    arrayValues[{"PLAYER_TALENT_SMITH", 0}] = 1;
    Check(Inspect(A::OreWeapon, {0}).status == Status::Available);
    flags["CURRENTLEVEL"] = 8;
    Check(Inspect(A::OreWeapon, {0}).status == Status::Later);
    flags["CURRENTLEVEL"] = 29;
    Check(Inspect(A::OreWeapon, {1}).status == Status::Running);
    Check(Inspect(A::OreWeapon, {2}).status == Status::Done);

    Check(Inspect(A::Bully, {0, Journal::Running}).status == Status::Running);
    Check(Inspect(A::Bully, {3}).status == Status::Unknown);
    dialogs["DIA_KNUT_DUELL"] = 1;
    dialogs["DIA_KNUT_AFTERFIGHT"] = 1;
    Check(Inspect(A::Bully, {3}).status == Status::Missed);

    actors.insert("PAL_99002_GERNOT");
    dialogs["DIA_GERNOT_AUSGUCK"] = 0;
    flags["MIS_KHORUSPROVIANT"] = 1;
    Check(Inspect(A::Lookout, {0}).status == Status::Later);
    flags["MIS_KHORUSPROVIANT"] = 2;
    Check(Inspect(A::Lookout, {0}).status == Status::Available);
    flags["CURRENTLEVEL"] = 57;
    Check(Inspect(A::Lookout, {0}).status == Status::Later);
    Check(Inspect(A::Lookout, {2}).status == Status::Done);

    items.insert("ITWR_SEAMAP_RAUHBEINSSCHATZKARTE");
    flags["JONESSCHATZKARTEGEFUNDEN"] = 0;
    Check(Inspect(A::PirateGold, {0}).status == Status::Available);
}

static void TestDeparture() {
    using namespace OQIDeparture;
    namespace A = OQIAlbatros;
    namespace T = OQITamora;
    Progress progress;
    progress.Add(Step::Deferred);
    progress.Add(Step::Missed);
    progress.Add(Step::Unknown);
    Check(!progress.Percent());
    Check(progress.missed == 1 && progress.unknown == 1 && progress.deferred == 1);
    progress.Add(Step::Done);
    progress.Add(Step::Done);
    progress.Add(Step::Open);
    Check(progress.Percent() == 66 && progress.Total() == 3);
    Check(NeedsReview(Step::Open) && NeedsReview(Step::Missed) && NeedsReview(Step::Unknown));
    Check(!NeedsReview(Step::Deferred) && !NeedsReview(Step::Done));

    flags.clear();
    dialogs.clear();
    actors.clear();
    items.clear();
    quantities.clear();

    chosenLocation = Location::Kondehra;
    Check(DepartureStep(Ring, {1}, Status::Running) == Step::Unknown);
    flags["READ_OLDSTORY1"] = 0;
    Check(DepartureStep(Ring, {1}, Status::Running) == Step::Open);
    flags["READ_OLDSTORY1"] = 1;
    Check(DepartureStep(Ring, {1}, Status::Running) == Step::Done);
    actors.insert("MIL_8101_SANDROCK");
    Check(DepartureStep(Rats, {0}, Status::Later) == Step::Open);
    Check(DepartureStep(Rats, {3}, Status::Missed) == Step::Missed);
    Check(DepartureStep(Provisions, {1}, Status::Running) == Step::Unknown);
    quantities = {{"ITFO_BACON", 10}, {"ITFO_CHEESE", 10}, {"ITFO_TOMATE", 10},
                  {"ITMI_MEHLSACK", 5}, {"ITMI_WEINFASS", 1}};
    Check(DepartureStep(Provisions, {1}, Status::Running) == Step::Open);
    flags["MIS_GESUNKENESSCHIFF"] = 2;
    Check(DepartureStep(Island, {1}, Status::Running) == Step::Done);

    chosenLocation = Location::Tamora;
    Check(DepartureStep(T::Blade, {1}, Status::Running) == Step::Deferred);
    Check(DepartureStep(T::Miners, {1}, Status::Running) == Step::Deferred);
    flags["KAPITEL"] = 2;
    flags["CURRENTLEVEL"] = 14;
    actors.insert("VLK_7710_BUERGERIN");
    dialogs["DIA_7710_BUERGERIN_STATTHALTER"] = 0;
    dialogs["DIA_7710_BUERGERIN_HILFE"] = 0;
    Check(Inspect(T::Dress, {0}).status == Status::Later);
    Check(DepartureStep(T::Dress, {0}, Status::Later) == Step::Open);
    dialogs["DIA_7710_BUERGERIN_STATTHALTER"] = 1;
    Check(Inspect(T::Dress, {0}).status == Status::Available);
    const auto dressOfferStep = DepartureStep(T::Dress, {0}, Status::Available);
    Check(dressOfferStep == Step::Open && NeedsReview(dressOfferStep));
    dialogs["DIA_7710_BUERGERIN_HILFE"] = 1;
    const Facts dressTaken{1, Journal::Running};
    const auto dressStep = DepartureStep(T::Dress, dressTaken, Inspect(T::Dress, dressTaken).status);
    Check(dressStep == Step::Deferred && !NeedsReview(dressStep));
    Progress tamoraProgress;
    for (int i = 0; i < 35; ++i) tamoraProgress.Add(Step::Done);
    auto progressBeforeOffer = tamoraProgress;
    progressBeforeOffer.Add(dressOfferStep);
    Check(progressBeforeOffer.Percent() == 97 && progressBeforeOffer.remaining == 1);
    tamoraProgress.Add(dressStep);
    Check(tamoraProgress.Percent() == 100 && tamoraProgress.remaining == 0);
    items.insert("ITAR_QUEEN");
    Check(DepartureStep(T::Dress, dressTaken, Status::Running) == Step::Open);
    actors.erase("VLK_7710_BUERGERIN");
    Check(DepartureStep(T::Dress, dressTaken, Status::Running) == Step::Deferred);
    items.erase("ITAR_QUEEN");
    Check(DepartureStep(T::Dress, {2}, Status::Done) == Step::Done);
    flags["PIRATENFLOSS_TAMORA_ENTDECKT"] = 0;
    Check(DepartureStep(T::Goblins, {1}, Status::Running) == Step::Open);
    flags["PIRATENFLOSS_TAMORA_ENTDECKT"] = 1;
    Check(DepartureStep(T::Goblins, {1}, Status::Running) == Step::Done);
    flags = {{"MIS_WEGZUBRUNO", 2}, {"MIS_FULKO", 2}, {"MIS_ORKSOELDNERTAMORA", 2},
             {"MIS_TAMORAMINEN", 2}, {"MIS_FLUCHFRIEDHOF", 2}};
    Check(DepartureStep(T::Mission, {1}, Status::Running) == Step::Done);

    chosenLocation = Location::Albatros;
    flags = {{"CURRENTLEVEL", 29}, {"SEAWORLD_ZEN", 16}, {"SEAWORLD2_ZEN", 29},
             {"SEAWORLD3_ZEN", 64}, {"SEAWORLDFIGHT_ZEN", 8}, {"MIS_KHORUSPROVIANT", 0}};
    Check(DepartureStep(A::Merchant, {1}, Status::Running) == Step::Deferred);
    Check(DepartureStep(A::Lookout, {0}, Status::Later) == Step::Deferred);
    flags["MIS_KHORUSPROVIANT"] = 2;
    Check(DepartureStep(A::Lookout, {0}, Status::Available) == Step::Open);
    flags["SCHMIED_TEACHSMITH"] = 0;
    actors.insert("MIL_9008_HAWK");
    Check(DepartureStep(A::OreWeapon, {0}, Status::Later) == Step::Open);
    Check(DepartureStep(A::OreWeapon, {0}, Status::Available) == Step::Open);
    quantities["ITMI_NUGGET"] = 2;
    packedQuantities["ITMI_NUGGET"] = 2;
    Check(DepartureStep(A::OreWeapon, {1}, Status::Running) == Step::Deferred);
    packedQuantities["ITMI_NUGGET"] = 3;
    Check(DepartureStep(A::OreWeapon, {1}, Status::Running) == Step::Open);
    actors.erase("MIL_9008_HAWK");
    flags["CURRENTLEVEL"] = 8;
    Check(DepartureStep(A::OreWeapon, {0}, Status::Later) == Step::Deferred);
    actors.insert("MIL_900801_HAWK");
    Check(DepartureStep(A::OreWeapon, {1}, Status::Running) == Step::Open);
    Check(DepartureStep(A::OreWeapon, {2}, Status::Done) == Step::Done);
    actors.insert("PAL_99002_GERNOT");
    Check(DepartureStep(A::Lookout, {1}, Status::Running) == Step::Open);
    Check(DepartureStep(A::Lookout, {2}, Status::Done) == Step::Done);
}

static void TestWalkthroughCoverage() {
    const auto check = [](const auto& entries) {
        for (const auto& quest : entries) {
            Check(quest.walkthrough != nullptr);
            const std::wstring_view steps = quest.walkthrough;
            const auto split = steps.find(L'\n');
            Check(split != std::wstring_view::npos && split > 0 && split + 1 < steps.size());
            Check(steps.find(L"\n\n") == std::wstring_view::npos);
        }
    };
    check(OQIKondehra::quests);
    check(OQIAlbatros::quests);
    check(OQITamora::quests);

    const std::wstring_view chronicles = OQITamora::quests[OQITamora::Chronicles].walkthrough;
    for (const auto* volume : {L"Том I.", L"Том II.", L"Том III.", L"Том IV.", L"Том V."}) {
        Check(chronicles.find(volume) != std::wstring_view::npos);
    }
}

int main() {
    TestTamora();
    TestAlbatros();
    TestDeparture();
    TestWalkthroughCoverage();
}
