#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "kondehra.hpp"
#include "quest_manifest.hpp"
#include "tamora.hpp"

namespace GOTHIC_NAMESPACE {
namespace OQI {

using namespace OQIKondehra;

static bool opened = false;
static bool loading = false;
static bool verifiedBuild = false;
static bool beforeDeparture = false;
static bool diagnostics = false;
static int questFilter = 0;
static int rawFilter = 0;
static int page = 0;
static int selected = 0;
enum class IslandView { Kondehra, Tamora };
static IslandView islandView = IslandView::Kondehra;
static std::optional<std::size_t> revealedQuest;
static std::chrono::steady_clock::time_point nextRefresh;

struct RawItem { std::string name; int value; };
struct QuestItem { std::string title; Result result; };
static std::vector<RawItem> rawItems;
static std::vector<QuestItem> questItems;

static const Quest& CatalogQuest(std::size_t index) {
    return islandView == IslandView::Tamora ? OQITamora::quests[index] : quests[index];
}

static std::size_t CatalogSize() {
    return islandView == IslandView::Tamora ? OQITamora::quests.size() : quests.size();
}

static zCPar_Symbol* Scalar(const char* name, unsigned int type) {
    auto* symbol = parser->GetSymbol(name);
    return symbol && symbol->type == type && symbol->ele == 1
        && !symbol->HasFlag(zPAR_FLAG_CLASSVAR) ? symbol : nullptr;
}

static std::optional<int> Value(const char* name) {
    auto* symbol = Scalar(name, zPAR_TYPE_INT);
    if (!symbol) return std::nullopt;
    int value = 0;
    symbol->GetValue(value, 0);
    return value;
}

static std::string TopicTitle(const char* name) {
    auto* symbol = Scalar(name, zPAR_TYPE_STRING);
    if (!symbol || !symbol->HasFlag(zPAR_FLAG_CONST)) return {};
    zSTRING value;
    symbol->GetValue(value, 0);
    return value.ToChar();
}

static bool IsVerifiedScriptBuild() {
    if (parser->GetIndex("TOPIC_KONDEHRAMISSION") != 13927
        || parser->GetIndex("KAPITEL") != 17077
        || parser->GetIndex("MIS_KOROFSKITESTAMENT") != 20329) return false;

    // Positions and FNV-1a fingerprints from the Russian 2.7.5.2 GOTHIC.DAT.
    struct Function { const char* name; int offset; int length; std::uint32_t hash; };
    static constexpr Function functions[] = {
        {"DIA_GERNOT_KONDEHRA_CONDITION", 8982763, 75, 0xf420982f},
        {"DIA_OLDERSON_GESUNKENESSCHIFF_CONDITION", 5898028, 67, 0xff3184c0},
        {"DIA_SANDROCK_SONSTALLESOK_NEIN", 7765381, 112, 0x6af1a46d},
        {"DIA_GYGES_ZEIGEHIRTENSTAB_NEIN", 5892037, 77, 0xe5496f40},
        {"DIA_GRISELDA_WILLDRAUSENESSEN_ICHPALADIN", 5902239, 273, 0xe8147eae},
        {"USEKOROFSKISTAGEBUCH", 1222490, 581, 0x4eb12588},
        {"TSZUGBRUECKEGESEHEN", 9847219, 123, 0x13380a36},
        {"DIA_GERNOT_TAMORA_CONDITION", 8983820, 63, 0xe6e34f2c},
        {"DIA_PAL_3_BRUNO_MINEN_CONDITION", 8870316, 39, 0x08114451},
        {"DIA_GERNOT_ADARMOS_INFO", 8985128, 283, 0xc7d81b52},
        {"VERLASSENESHAUS_FUNC", 9857931, 100, 0xa8a435f1}
    };
    const auto& code = parser->stack;
    for (const auto& function : functions) {
        auto* symbol = parser->GetSymbol(function.name);
        if (!symbol || symbol->type != zPAR_TYPE_FUNC || !symbol->HasFlag(zPAR_FLAG_CONST)) return false;
        int position = -1;
        symbol->GetStackPos(position, 0);
        if (position != function.offset || !code.stack || code.stacksize < position + function.length) return false;
        std::uint32_t hash = 2166136261u;
        for (int i = 0; i < function.length; ++i) {
            hash = (hash ^ code.stack[position + i]) * 16777619u;
        }
        if (hash != function.hash) return false;
    }
    return true;
}

static Journal ReadJournal(const std::string& title) {
    if (title.empty()) return Journal::Other;
    const auto& topics = oCLogManager::GetLogManager().m_lstTopics;
    auto state = Journal::Absent;
    for (auto* node = topics.GetNextInList(); node; node = node->GetNextInList()) {
        const auto* topic = node->GetData();
        if (!topic || topic->m_enuSection != oCLogTopic::zELogSection_Missions
            || title != topic->m_strDescription.ToChar()) continue;
        if (state != Journal::Absent) return Journal::Other;
        switch (topic->m_enuStatus) {
        case oCLogTopic::zELogTopicStatus_Running: state = Journal::Running; break;
        case oCLogTopic::zELogTopicStatus_Success: state = Journal::Success; break;
        case oCLogTopic::zELogTopicStatus_Failure: state = Journal::Failure; break;
        case oCLogTopic::zELogTopicStatus_Obsolete: state = Journal::Obsolete; break;
        default: state = Journal::Other; break;
        }
    }
    return state;
}

static std::optional<int> Told(const char* name) {
    const int index = parser->GetIndex(name);
    auto* manager = ogame->GetInfoManager();
    if (index < 0 || !manager) return std::nullopt;
    auto* info = manager->GetInformation(index);
    if (!info) return std::nullopt;
    return info->told != 0;
}

static oCNpc* FindLivingNpc(const char* name) {
    const int index = parser->GetIndex(name);
    if (index < 0) return nullptr;
    for (auto* node = ogame->GetGameWorld()->voblist_npcs; node; node = node->GetNextInList()) {
        auto* npc = node->GetData();
        if (npc && npc->GetInstance() == index && !npc->IsDead()) return npc;
    }
    return nullptr;
}

static bool OnKondehra() {
    return Value("CURRENTLEVEL") == 57; // KONDEHRA_ZEN in the verified script build.
}

static bool OnTamora() {
    const auto tamora = Value("TAMORA_ZEN");
    return tamora && Value("CURRENTLEVEL") == tamora;
}

static bool HasUnpackedItem(const char* name) {
    const int index = parser->GetIndex(name);
    if (index < 0) return false;
    const auto& items = player->inventory2.inventory;
    for (auto* node = items.GetNextInList(); node; node = node->GetNextInList()) {
        auto* item = node->GetData();
        if (item && item->GetInstance() == index && item->amount > 0) return true;
    }
    return false;
}

static Start Require(std::initializer_list<std::pair<std::optional<int>, int>> checks) {
    bool incomplete = false;
    for (const auto& check : checks) {
        if (check.first && *check.first != check.second) return Start::Later;
        if (!check.first) incomplete = true;
    }
    return incomplete ? Start::Unknown : Start::Yes;
}

static Start WithNpc(Start ready, const char* name) {
    if (ready != Start::Yes) return ready;
    return FindLivingNpc(name) ? Start::Yes : Start::Unknown;
}

static Start StartRule(QuestId quest) {
    const auto chapter = Value("KAPITEL");
    if (!chapter) return Start::Unknown;
    if (*chapter < 2) return Start::Later;
    if (*chapter != 2) return Start::Unknown;

    if (quest == Island || quest == Fleet) {
        const auto shipOffer = Require({
            {Told("DIA_GERNOT_KONDEHRA"), 0}, {Told("DIA_GERNOT_SEXTANTGEBEN"), 1},
            {Value("MIS_WOLFGANG_DIEBSTAHL"), 2}, {Value("MIS_DANDOLO_STRANDGUT"), 2},
            {Value("MIS_SVEN_SUMPFKRAUTHANDEL"), 2}
        });
        if (shipOffer == Start::Yes && !OnKondehra()) {
            return WithNpc(shipOffer, "PAL_99002_GERNOT");
        }
        if (quest == Fleet && OnKondehra()) {
            return WithNpc(Require({{Told("DIA_SANDROCK_WERBISTDU"), 1},
                                    {Told("DIA_SANDROCK_ORKS"), 0}}), "MIL_8101_SANDROCK");
        }
        return shipOffer == Start::Yes ? Start::Unknown : shipOffer;
    }

    // The original item callbacks can also run after the hero leaves the island.
    // Observe unpacked items without calling inventory functions that unpack them.
    switch (quest) {
    case Goblins:
        if ((HasUnpackedItem("ITWR_PIRATENGOBLINFLOSS") && Value("PIRATENGOBLINSBUCH2GELESEN") == 0)
            || (HasUnpackedItem("ITWR_PIRATENGOBLINSCHATZ") && Value("PIRATENGOBLINSBUCHGELESEN") == 0
                && Value("PIRATENGOBLINSSCHATZGEFUNDEN") == 0)) return Start::Yes;
        return Start::Unknown;
    case Ring:
        return HasUnpackedItem("ITWR_OLDSTORY1") && Value("READ_OLDSTORY1") == 0
            ? Start::Yes : Start::Unknown;
    case Testament:
        return HasUnpackedItem("ITWR_KOROFSKISTAGEBUCH") && Value("KOROFSKITAGEBUCH1GELESEN") == 0
            ? Start::Yes : Start::Unknown;
    default:
        break;
    }

    if (!Value("CURRENTLEVEL")) return Start::Unknown;
    if (!OnKondehra()) return Start::Later;

    switch (quest) {
    case Shrine:
        return WithNpc(Require({{Told("DIA_OSKAR_EINPALADIN"), 0}}), "BAU_8103_OSKAR");
    case Couple:
        return Told("DIA_HEIDELINDE_WASBISTDUFUEREINER") == 0
            ? WithNpc(Start::Yes, "BAU_8107_HEIDELINDE") : Start::Unknown;
    case Wolf:
        return WithNpc(Require({{Told("DIA_GYGES_WEISTDUINNOSSCHREIN"), 1},
                                {Told("DIA_GYGES_PROBLEM"), 0}}), "BAU_8102_GYGES");
    case Errand:
        return WithNpc(Require({{Told("DIA_GRISELDA_FRAGWASESSEN"), 0}}), "BAU_8106_GRISELDA");
    case Fortress:
        return Require({{Value("ZUGBRUECKEGESEHEN"), 0}, {Told("DIA_SANDROCK_HALLO"), 0}});
    case Library:
        return WithNpc(Require({{Told("DIA_SANDROCK_UEBERDIEFESTUNG"), 1},
                                {Told("DIA_SANDROCK_WEITERMITFESTUNG"), 0}}), "MIL_8101_SANDROCK");
    case Rats:
        return WithNpc(Require({{Told("DIA_SANDROCK_WEITERMITFESTUNG"), 1},
                                {Told("DIA_SANDROCK_SONSTALLESOK"), 0}}), "MIL_8101_SANDROCK");
    case Wreck:
        return WithNpc(Require({{Told("DIA_OLDERSON_GESUNKENESSCHIFF"), 0},
                                {Value("MIS_INNOSSCHREIN"), 2}, {Value("MIS_ORKFLOTTE"), 1},
                                {Told("DIA_SANDROCK_ORKS"), 1}, {Told("DIA_OLDERSON_SCHREINISTWIEDERDA"), 1}}),
                       "BAU_8104_OLDERSON");
    case Staff: {
        // The script sets LOG_RUNNING before Gyges reaches the spot and asks for help.
        const auto ready = Require({{Value("MIS_GYGESHIRTENSTAB"), 1},
                                    {Told("DIA_GYGES_ZEIGEHIRTENSTAB"), 0}});
        if (ready != Start::Yes) return ready;
        auto* npc = FindLivingNpc("BAU_8102_GYGES");
        auto* wayNet = ogame->GetGameWorld()->wayNet;
        auto* waypoint = wayNet ? wayNet->GetWaypoint("WP_OIK_PLAT1_PIE") : nullptr;
        if (!npc || !waypoint) return Start::Unknown;
        auto pos = waypoint->GetPositionWorld();
        return (npc->GetPositionWorld() - pos).Length() <= 200.0f ? Start::Yes : Start::Later;
    }
    case Provisions:
        return WithNpc(Require({{Told("DIA_SANDROCK_ABSCHIED"), 1},
                                {Told("DIA_SANDROCK_PROVIANTFUERKONDEHRA"), 0},
                                {Value("MIS_ORKFLOTTE"), 1}}), "MIL_8101_SANDROCK");
    default:
        // A document or encounter in the world can start these. Its current
        // reachability cannot be established from the tracked flags.
        return Start::Unknown;
    }
}

static bool ClosedRule(QuestId quest, std::optional<int> flag) {
    switch (quest) {
    case Rats: return flag == 3;
    case Staff: return flag == 4;
    case Fortress:
        return flag == 0 && Told("DIA_SANDROCK_HALLO") == 1
            && Value("ZUGBRUECKEGESEHEN") == 0;
    case Testament:
        return flag == 3 && Value("KOROFSKITAGEBUCH1GELESEN") == 1;
    default: return false;
    }
}

// Tamora-specific start and closing conditions are defined beside the catalog.
#include "tamora_rules.hpp"

static void RefreshQuests() {
    nextRefresh = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    verifiedBuild = IsVerifiedScriptBuild();
    if (!verifiedBuild) return;

    questItems.resize(CatalogSize());
    for (std::size_t i = 0; i < questItems.size(); ++i) {
        const auto& quest = CatalogQuest(i);
        auto& item = questItems[i];
        item.title = TopicTitle(quest.topic);
        Facts facts;
        facts.flag = Value(quest.flag);
        facts.journal = ReadJournal(item.title);
        if (islandView == IslandView::Tamora) {
            const auto id = static_cast<OQITamora::QuestId>(i);
            facts.windowClosed = TamoraClosedRule(id, facts);
            if (facts.flag == 0) facts.start = TamoraStartRule(id);
            item.result = Evaluate(facts);
        } else {
            const auto id = static_cast<QuestId>(i);
            facts.windowClosed = ClosedRule(id, facts.flag);
            facts.staffAccepted = id == Staff && Value("HIRTENSTABRUN") == 1;
            if (facts.flag == 0 || (id == Staff && !facts.staffAccepted && facts.flag == 1)) {
                facts.start = StartRule(id);
            }
            item.result = Evaluate(id, facts);
        }
    }
}

static void RefreshRaw() {
    rawItems.clear();
    page = 0;
    for (auto* name : OQIManifest::kNames) {
        const auto value = Value(name);
        if (value) rawItems.push_back({name, *value});
    }
}

static constexpr int leftX = 260;
static constexpr int hintX = 4510;
static constexpr int rightEdge = 7950;
static constexpr int topY = 300;

static int RowHeight() {
    auto* font = screen->GetFont();
    return font ? (std::max)(310, screen->any(font->GetFontY()) + 30) : 310;
}

static int TextWidth(const std::string& text) {
    auto* font = screen->GetFont();
    return font ? screen->anx(font->GetFontX(zSTRING(text.c_str()))) : 0;
}

static void Text(int row, const char* text, int x = leftX) {
    screen->Print(x, topY + row * RowHeight(), zSTRING(text));
}

static std::string FitText(std::string text, int width) {
    if (TextWidth(text) <= width) return text;
    while (!text.empty() && TextWidth(text + "...") > width) text.pop_back();
    return text + "...";
}

static int WrappedText(int row, const std::string& text) {
    const int width = rightEdge - hintX;
    const int rowLimit = (7600 - topY) / RowHeight();
    std::string line;
    for (std::size_t start = 0; start < text.size() && row <= rowLimit;) {
        auto end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        const auto word = text.substr(start, end - start);
        const auto candidate = line.empty() ? word : line + ' ' + word;
        if (!line.empty() && TextWidth(candidate) > width) {
            Text(row++, FitText(line, width).c_str(), hintX);
            line = word;
        } else {
            line = candidate;
        }
        start = end + 1;
    }
    if (!line.empty() && row <= rowLimit) Text(row++, FitText(line, width).c_str(), hintX);
    return row;
}

static std::string Russian(const wchar_t* value) {
    char buffer[640] = {};
    WideCharToMultiByte(1251, 0, value, -1, buffer, sizeof(buffer), nullptr, nullptr);
    return buffer;
}

static void Text(int row, const wchar_t* value) {
    Text(row, Russian(value).c_str());
}

static const wchar_t* StatusText(Status status) {
    switch (status) {
    case Status::Available: return L"Доступно, не начато";
    case Status::Running: return L"Выполняется";
    case Status::Done: return L"Выполнено";
    case Status::Later: return L"Пока недоступно";
    case Status::Missed: return L"Пропущено";
    default: return L"Не удалось определить";
    }
}

static const wchar_t* Reason(std::size_t id, Evidence evidence) {
    switch (evidence) {
    case Evidence::Journal: return L"Статус подтверждён журналом.";
    case Evidence::Script: return L"Стадия подтверждена скриптом.";
    case Evidence::StartRule: return L"Проверены условия начала.";
    case Evidence::ClosedRule:
        if (islandView == IslandView::Tamora) {
            const auto* reason = CatalogQuest(id).caution;
            return reason ? reason : L"Условие закрытия подтверждено.";
        }
        switch (id) {
        case Fortress: return L"Знакомство с Сандроком закрыло задание у моста.";
        case Testament: return L"Началось поручение в Варанте. Завещание уже закрыто.";
        default: return L"Вы отказались в диалоге. Предложение больше не появится.";
        }
    default: return L"Текущее состояние или доступность не подтверждены.";
    }
}

static const wchar_t* WindowNotice(QuestId id) {
    switch (id) {
    case Fortress: return L"Сначала подойдите к мосту. Знакомство с Сандроком закроет этот старт.";
    case Rats: return L"Отказ у Сандрока закрывает поручение.";
    case Staff: return L"Отказ у Гигеса закрывает поручение.";
    case Testament: return L"Поручение Болдуина в Варанте прерывает начатую цепочку.";
    case Errand: return L"При отказе игра ставит провал. Сдать суп может быть ещё возможно.";
    default: return nullptr;
    }
}

static const wchar_t* Chain(QuestId id) {
    switch (id) {
    case Island:
    case Wreck: return L"Алтарь Инноса > затонувший корабль > доклад Герноту";
    case Wolf:
    case Shrine: return L"Серый волк > подсказка Гигеса об алтаре > посох и корабль";
    case Fleet:
    case Provisions: return L"Флот орков > затонувший корабль > прощание с Сандроком > провиант";
    case Library:
    case Rats: return L"Разговор о крепости > библиотека и поручение о крысах";
    default: return nullptr;
    }
}

static constexpr int questFilterCount = 7;
static constexpr int rawFilterCount = 5;
static constexpr int rawPageSize = 13;

static void PaintQuests(int movement, bool toggleHints) {
    const bool tamora = islandView == IslandView::Tamora;
    const auto heading = Russian(tamora ? L"Тамора · Глава 2 · " : L"Кондера · Глава 2 · ")
        + Russian(beforeDeparture ? L"Перед отплытием" : L"Задания");
    Text(0, heading.c_str());
    Text(1, "Shift+F: FILTER | Shift+V: DEPARTURE | Shift+D: FLAGS | R");
    Text(2, L"PgUp/PgDn: выбор | Shift+H: подсказка (спойлер) | Shift+T: остров");
    if (!verifiedBuild) {
        Text(3, L"Скрипты русской версии 2.7.5.2 не распознаны.");
        Text(4, L"Доступен просмотр значений по Shift+D.");
        return;
    }

    const auto chapter = Value("KAPITEL");
    if (chapter != 2 || !(tamora ? OnTamora() : OnKondehra())) {
        Text(3, L"Вы сейчас вне выбранной локации или главы.");
    } else if (beforeDeparture) {
        Text(3, L"Проверьте оставшиеся дела. Звёздочкой отмечены задания с продолжением вне острова.");
    }

    std::vector<std::size_t> visible;
    for (std::size_t i = 0; i < questItems.size(); ++i) {
        const auto status = questItems[i].result.status;
        if (beforeDeparture && !IncludeBeforeDeparture(status)) continue;
        if (questFilter != 0 && static_cast<int>(status) + 1 != questFilter) continue;
        visible.push_back(i);
    }

    const wchar_t* filter = questFilter ? StatusText(static_cast<Status>(questFilter - 1)) : L"Все";
    Text(4, (Russian(L"Фильтр: ") + Russian(filter)).c_str());
    if (visible.empty()) {
        revealedQuest.reset();
        Text(5, L"Для этого фильтра заданий нет.");
        return;
    }
    const auto oldSelection = selected;
    selected = std::clamp(selected + movement, 0, static_cast<int>(visible.size()) - 1);
    const auto selectedId = visible[selected];
    if (oldSelection != selected || revealedQuest != selectedId) revealedQuest.reset();
    if (toggleHints) {
        if (revealedQuest) revealedQuest.reset();
        else revealedQuest = selectedId;
    }
    const bool showHints = revealedQuest.has_value();

    const int rows = std::clamp((7150 - topY) / RowHeight() - 5, 1, 15);
    const int first = (selected / rows) * rows;
    const int end = (std::min)(first + rows, static_cast<int>(visible.size()));
    const auto indicator = std::to_string(first + 1) + "-" + std::to_string(end)
        + " / " + std::to_string(visible.size());
    Text(4, indicator.c_str(), 3600);
    for (int i = first; i < end; ++i) {
        const auto id = visible[i];
        const auto& item = questItems[id];
        std::string title = i == selected ? "> " : "  ";
        title += item.title.empty() ? Russian(L"Название отсутствует") : item.title;
        if (CatalogQuest(id).continuesBeyondIsland) title += " *";
        const auto status = Russian(StatusText(item.result.status));
        const int row = 5 + i - first;
        if (showHints) {
            const auto fittedStatus = FitText(status, (hintX - leftX) / 2);
            const int statusX = hintX - 140 - TextWidth(fittedStatus);
            Text(row, FitText(title, statusX - leftX - 120).c_str());
            Text(row, fittedStatus.c_str(), statusX);
        } else {
            Text(row, FitText(title + " | " + status, rightEdge - leftX).c_str());
        }
    }
    if (!showHints) return;

    const auto& item = questItems[selectedId];
    const auto& quest = CatalogQuest(selectedId);
    int row = WrappedText(4, item.title.empty() ? Russian(L"Подсказка") : item.title);
    row = WrappedText(row + 1, Russian(Reason(selectedId, item.result.evidence)));
    row = WrappedText(row + 1, Russian(quest.start));
    row = WrappedText(row + 1, Russian(quest.finish));
    const auto id = static_cast<QuestId>(selectedId);
    const wchar_t* notice = tamora ? quest.caution : WindowNotice(id);
    const wchar_t* chain = tamora ? quest.chain : Chain(id);
    if (notice) row = WrappedText(row + 1, Russian(notice));
    if (chain) WrappedText(row + 1, Russian(chain));
}

static void PaintRaw() {
    static constexpr const char* labels[] = {"ALL", "VALUE 1", "VALUE 2", "VALUE 0", "OTHER"};
    std::vector<const RawItem*> visible;
    for (const auto& item : rawItems) {
        int match = item.value == 1 ? 1 : item.value == 2 ? 2 : item.value == 0 ? 3 : 4;
        if (!rawFilter || rawFilter == match) visible.push_back(&item);
    }
    const int count = static_cast<int>(visible.size());
    const int pages = (std::max)(1, (count + rawPageSize - 1) / rawPageSize);
    page = std::clamp(page, 0, pages - 1);
    char line[256];
    std::snprintf(line, sizeof(line), "ODYSSEE QUEST INSPECTOR | %s | %d/%d",
                  labels[rawFilter], page + 1, pages);
    Text(0, line);
    std::snprintf(line, sizeof(line), "RAW FLAGS %d/%d", static_cast<int>(rawItems.size()), OQIManifest::kCount);
    Text(1, line);
    Text(2, "Shift+F: FILTER | Shift+D: QUESTS | LEFT/RIGHT: PAGE | R: REFRESH");
    const int begin = page * rawPageSize;
    const int end = (std::min)(begin + rawPageSize, count);
    for (int i = begin; i < end; ++i) {
        std::snprintf(line, sizeof(line), "[%d] %s", visible[i]->value, visible[i]->name.c_str());
        Text(4 + i - begin, line);
    }
    if (!count) Text(4, "NO MATCHING FLAGS");
}

struct Key {
    int code;
    bool down = false;

    bool Pressed() {
        const bool now = zinput->KeyPressed(code) != 0;
        const bool pressed = now && !down;
        down = now;
        return pressed;
    }
};

static Key filterKey{KEY_F}, departureKey{KEY_V}, diagnosticKey{KEY_D};
static Key previousKey{KEY_PGUP}, nextKey{KEY_PGDN}, hintKey{KEY_H}, islandKey{KEY_T};

static void Loop() {
    if (loading || !zinput || !player || !screen || !parser || !ogame || !ogame->GetGameWorld()) return;

    const bool shift = zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);
    const bool filterPressed = filterKey.Pressed() && shift;
    const bool departurePressed = departureKey.Pressed() && shift;
    const bool diagnosticPressed = diagnosticKey.Pressed() && shift;
    const bool hintPressed = hintKey.Pressed() && shift;
    const bool islandPressed = islandKey.Pressed() && shift;
    const int movement = static_cast<int>(nextKey.Pressed()) - static_cast<int>(previousKey.Pressed());

    if (zinput->KeyToggled(KEY_APOSTROPHE)) {
        opened = !opened;
        revealedQuest.reset();
        if (opened) {
            if (OnTamora() && islandView != IslandView::Tamora && questItems.empty()) islandView = IslandView::Tamora;
            if (diagnostics) RefreshRaw();
            else RefreshQuests();
        }
    }
    if (!opened) return;
    if (diagnosticPressed) {
        diagnostics = !diagnostics;
        revealedQuest.reset();
        if (diagnostics) RefreshRaw();
        else RefreshQuests();
    }
    const bool refreshPressed = zinput->KeyToggled(KEY_R);
    if (diagnostics) {
        if (filterPressed) { rawFilter = (rawFilter + 1) % rawFilterCount; page = 0; }
        if (refreshPressed) RefreshRaw();
        if (zinput->KeyToggled(KEY_LEFT)) --page;
        if (zinput->KeyToggled(KEY_RIGHT)) ++page;
        PaintRaw();
        return;
    }
    if (islandPressed) {
        islandView = islandView == IslandView::Tamora ? IslandView::Kondehra : IslandView::Tamora;
        questFilter = selected = 0;
        revealedQuest.reset();
        RefreshQuests();
    }
    if (departurePressed) { beforeDeparture = !beforeDeparture; questFilter = 0; selected = 0; revealedQuest.reset(); }
    if (filterPressed) { questFilter = (questFilter + 1) % questFilterCount; selected = 0; revealedQuest.reset(); }
    if (refreshPressed || std::chrono::steady_clock::now() >= nextRefresh) RefreshQuests();
    PaintQuests(movement, hintPressed && !islandPressed);
}

class LoadScope {
    bool wasLoading = loading;
public:
    LoadScope() {
        loading = true;
        opened = false;
        verifiedBuild = false;
        beforeDeparture = false;
        diagnostics = false;
        revealedQuest.reset();
        questFilter = rawFilter = page = selected = 0;
        rawItems.clear();
        questItems.clear();
        islandView = IslandView::Kondehra;
    }
    ~LoadScope() { loading = wasLoading; }
};

}  // namespace OQI

// The main-world render point is the G2A address supplied by the Union template.
void __fastcall OQI_Render(Union::Registers&);
auto OQI_RenderHook = Union::CreatePartialHook(
    reinterpret_cast<void*>(zSwitch(0x0, 0x0, 0x0, 0x006C87EB)), &OQI_Render);

void __fastcall OQI_Render(Union::Registers&) { OQI::Loop(); }

void __fastcall OQI_LoadGame(oCGame*, void*, int, const zSTRING&);
auto OQI_LoadGameHook = Union::CreateHook(
    SIGNATURE_OF(&oCGame::LoadGame), &OQI_LoadGame, Union::HookType::Hook_Detours);

void __fastcall OQI_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath) {
    OQI::LoadScope load;
    OQI_LoadGameHook(self, vtable, slot, levelPath);
}

void __fastcall OQI_LoadSavegame(oCGame*, void*, int, zBOOL);
auto OQI_LoadSavegameHook = Union::CreateHook(
    SIGNATURE_OF(&oCGame::LoadSavegame), &OQI_LoadSavegame, Union::HookType::Hook_Detours);

void __fastcall OQI_LoadSavegame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals) {
    OQI::LoadScope load;
    OQI_LoadSavegameHook(self, vtable, slot, loadGlobals);
}

void __fastcall OQI_ChangeLevel(oCGame*, void*, const zSTRING&, const zSTRING&);
auto OQI_ChangeLevelHook = Union::CreateHook(
    SIGNATURE_OF(&oCGame::ChangeLevel), &OQI_ChangeLevel, Union::HookType::Hook_Detours);

void __fastcall OQI_ChangeLevel(oCGame* self, void* vtable, const zSTRING& level, const zSTRING& start) {
    OQI::LoadScope load;
    OQI_ChangeLevelHook(self, vtable, level, start);
}

}  // namespace GOTHIC_NAMESPACE
