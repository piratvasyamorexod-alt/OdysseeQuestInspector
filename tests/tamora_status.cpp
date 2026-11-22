#include "../src/tamora.hpp"

#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

using namespace OQIKondehra;

static std::map<std::string, int> flags{{"KAPITEL", 2}, {"CURRENTLEVEL", 14}};
static std::map<std::string, int> dialogs;
static std::set<std::string> actors;
static std::set<std::string> items;

static std::optional<int> Read(const std::map<std::string, int>& values, const char* name) {
    const auto it = values.find(name);
    if (it == values.end()) return std::nullopt;
    return it->second;
}

static std::optional<int> Value(const char* name) { return Read(flags, name); }
static std::optional<int> Told(const char* name) { return Read(dialogs, name); }
static bool OnTamora() { return Value("CURRENTLEVEL") == 14; }
static bool HasUnpackedItem(const char* name) { return items.count(name) != 0; }
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

#include "../src/tamora_rules.ipp"

static void Check(bool condition) {
    if (!condition) std::abort();
}

static Result Inspect(OQITamora::QuestId id, Facts facts) {
    facts.windowClosed = TamoraClosedRule(id, facts);
    if (facts.flag == 0) facts.start = TamoraStartRule(id);
    return Evaluate(facts);
}

int main() {
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
