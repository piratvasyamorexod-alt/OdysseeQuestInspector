#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "albatros.hpp"
#include "departure.hpp"
#include "journal_mouse_win32.hpp"
#include "kondehra.hpp"
#include "quest_manifest.hpp"
#include "tamora.hpp"

namespace GOTHIC_NAMESPACE {
namespace OQI {

using namespace OQIKondehra;

static bool loading = false;
static bool updatedFromInput = false;
static bool apostropheWasDown = false;
static bool verifiedBuild = false;
static bool beforeDeparture = false;
static bool diagnostics = false;
static int questFilter = 0;
static int rawFilter = 0;
static int page = 0;
static int selected = 0;
enum class Location : std::size_t { Albatros, Kondehra, Tamora };
static std::optional<Location> location;
static std::optional<std::size_t> revealedQuest;
static std::chrono::steady_clock::time_point nextRefresh;

struct RawItem { std::string name; int value; };
struct QuestItem {
    std::string title;
    Result result;
    OQIDeparture::Step step = OQIDeparture::Step::Unknown;
};
static OQIDeparture::Progress progress;
static bool currentVisit = false;
static std::vector<RawItem> rawItems;
static std::vector<QuestItem> questItems;

struct Catalog {
    const wchar_t* name;
    const Quest* entries;
    std::size_t size;
};

static constexpr Catalog catalogs[] = {
    {L"Альбатрос", OQIAlbatros::quests.data(), OQIAlbatros::quests.size()},
    {L"Кондера", quests.data(), quests.size()},
    {L"Тамора", OQITamora::quests.data(), OQITamora::quests.size()},
};

static Location SelectedLocation() {
    return location.value_or(Location::Kondehra);
}

static const Catalog& SelectedCatalog() {
    return catalogs[static_cast<std::size_t>(SelectedLocation())];
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
        {"VERLASSENESHAUS_FUNC", 9857931, 100, 0xa8a435f1},
        {"DIA_HARALD_JOBBOOTSMANN_CONDITION", 7769836, 27, 0xb2fbe492},
        {"DIA_GERNOT_AUSGUCK_CONDITION", 8979705, 23, 0x19abca6a},
        {"DIA_KNUT_AFTERFIGHT_INFO", 7863592, 375, 0xf6075ba9}
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

static bool OnAlbatros() {
    const auto level = Value("CURRENTLEVEL");
    if (!level) return false;
    return level == Value("SEAWORLD_ZEN") || level == Value("SEAWORLD2_ZEN")
        || level == Value("SEAWORLD3_ZEN") || level == Value("SEAWORLDFIGHT_ZEN");
}

static std::optional<Location> CurrentLocation() {
    if (OnKondehra()) return Location::Kondehra;
    if (OnTamora()) return Location::Tamora;
    if (OnAlbatros()) return Location::Albatros;
    return std::nullopt;
}

static int UnpackedAmount(const char* name) {
    const int index = parser->GetIndex(name);
    if (index < 0) return 0;
    int amount = 0;
    const auto& items = player->inventory2.inventory;
    for (auto* node = items.GetNextInList(); node; node = node->GetNextInList()) {
        auto* item = node->GetData();
        if (item && item->GetInstance() == index && item->amount > 0) amount += item->amount;
    }
    return amount;
}

static bool HasUnpackedItem(const char* name) {
    return UnpackedAmount(name) > 0;
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
#include "albatros_rules.hpp"
#include "departure_rules.hpp"

static void RefreshQuests() {
    nextRefresh = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    verifiedBuild = IsVerifiedScriptBuild();
    currentVisit = verifiedBuild && Value("KAPITEL") == 2 && CurrentLocation() == SelectedLocation();
    progress = {};
    if (!verifiedBuild) return;

    const auto& catalog = SelectedCatalog();
    questItems.resize(catalog.size);
    for (std::size_t i = 0; i < questItems.size(); ++i) {
        const auto& quest = catalog.entries[i];
        auto& item = questItems[i];
        item.title = TopicTitle(quest.topic);
        Facts facts;
        facts.flag = Value(quest.flag);
        facts.journal = ReadJournal(item.title);
        if (SelectedLocation() == Location::Albatros) {
            const auto id = static_cast<OQIAlbatros::QuestId>(i);
            facts.windowClosed = AlbatrosClosedRule(id, facts);
            if (facts.flag == 0) facts.start = AlbatrosStartRule(id);
            item.result = Evaluate(facts);
        } else if (SelectedLocation() == Location::Tamora) {
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
        if (currentVisit) {
            item.step = DepartureStep(i, facts, item.result.status);
            progress.Add(item.step);
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

struct JournalInput {
    int selection = 0;
    int pages = 0;
    int locations = 0;
    int hintLines = 0;
    int hintPages = 0;
    int mouseWheel = 0;
    bool mouseClick = false;
    bool hint = false;
    bool close = false;
};

struct JournalRect {
    int x, y, width, height;

    bool Contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

static constexpr char version[] = "1.1.1";
static constexpr unsigned long lineInk = 0xFF655740;
static constexpr unsigned long bronzeInk = 0xFFDDB67D;

static JournalRect JournalBounds() {
    const int width = (std::min)(screen->psizex * 92 / 100, screen->psizey * 114 / 100);
    const int height = screen->psizey * 70 / 100;
    return {screen->anx((screen->psizex - width) / 2),
            screen->any((screen->psizey - height) / 2),
            screen->anx(width), screen->any(height)};
}

struct JournalMouse {
    int x = 0, y = 0, wheel = 0;
    bool active = false, pressed = false;

    bool Over(const JournalRect& bounds) const { return active && bounds.Contains(x, y); }
};

static bool ShiftDown() {
    return zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);
}

static HWND ForegroundGameWindow() {
    const HWND foreground = GetForegroundWindow();
    DWORD process = 0;
    GetWindowThreadProcessId(foreground, &process);
    return process == GetCurrentProcessId() ? foreground : nullptr;
}

static bool WindowsKey(int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

class JournalView : public zCView {
    oCGame& game;
    OQIMouse::MouseEvents mouseEvents;
    OQIMouse::JournalPointer systemPointer;
    bool restoreEngineMouse = false;
    bool centerMouseOnOpen = true;
    zCView scrollView{0, 0, 8, 8};
    zCView cursorView{0, 0, 8, 8};
    std::optional<RECT> cursorClient;
    int cursorX = 0, cursorY = 0;
    bool mouseHadFocus = false;
    bool mouseDown = false;
    bool windowsKeys[8]{};
    unsigned mouseClicks = 0, mouseWheelSteps = 0;

    static void ApplyMouseAcquisition() {
        // G2A 2.6 fix SetAcquireMouse applies the engine flag to DirectInput.
        reinterpret_cast<int(__cdecl*)()>(0x004D3D30)();
    }

    void RestoreEngineMouse() {
        systemPointer.Leave();
        if (std::exchange(restoreEngineMouse, false) && !zinput->GetDeviceEnabled(zINPUT_MOUSE)) {
            zinput->SetDeviceEnabled(zINPUT_MOUSE, TRUE);
            ApplyMouseAcquisition();
        }
    }

    // Render these textures through the same engine path as the journal itself.
    static void DrawSprite(zCView& view, int x, int y, int width, int height, unsigned long ink) {
        view.pposx = x;
        view.pposy = y;
        view.psizex = width;
        view.psizey = height;
        zCOLOR color;
        color.dword = ink;
        view.SetColor(color);
        view.Blit();
    }

    static std::optional<POINT> CursorPixels(const RECT& client, POINT desktop) {
        // GetPixelSize only writes its outputs for a view with an owner.
        const int width = screen->psizex;
        const int height = screen->psizey;
        if (width <= 0 || height <= 0 || !PtInRect(&client, desktop)) return std::nullopt;
        return POINT{
            (std::min)(width - 1, MulDiv(desktop.x - client.left, width, client.right - client.left)),
            (std::min)(height - 1, MulDiv(desktop.y - client.top, height, client.bottom - client.top))
        };
    }

    void DrawRule(const JournalRect& rect, int inset, unsigned long ink) {
        const int x = nax(rect.x);
        const int y = nay(rect.y);
        const int width = nax(rect.x + rect.width) - x - 2 * inset;
        const int height = nay(rect.y + rect.height) - y;
        if (width <= 0 || height <= 0) return;
        DrawSprite(scrollView, pposx + x + inset, pposy + y, width, height, ink);
    }

    JournalView(oCGame& game, JournalRect box)
        : zCView(box.x, box.y, box.x + box.width, box.y + box.height), game(game) {
        SetFont(screen->GetFont());
        InsertBack(zSTRING("OQI_JOURNAL.TGA"));
        SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
        SetTransparency(248);
        scrollView.InsertBack(zSTRING("OQI_SCROLL.TGA"));
        scrollView.SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
        scrollView.SetTransparency(255);
        cursorView.InsertBack(zSTRING("OQI_CURSOR.TGA"));
        cursorView.SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
        cursorView.SetTransparency(255);
        game.Pause();
        screen->InsertItem(this);
        SetEnableHandleEvent(TRUE);
        SetHandleEventTop();
        TakeInput();
    }

public:
    struct ScrollBar {
        JournalRect track, thumb;
        bool highlighted;
    };

    JournalInput pending;
    std::optional<ScrollBar> listScrollBar, hintScrollBar;
    std::optional<int> listGrabOffset, hintGrabOffset;

    struct Rule {
        JournalRect bounds;
        unsigned long color;
    };

    std::vector<Rule> rules;

    explicit JournalView(oCGame& game) : JournalView(game, JournalBounds()) {}

    ~JournalView() override {
        SetEnableHandleEvent(FALSE);
        if (owner) owner->RemoveItem(this);
        mouseEvents.Stop();
        RestoreEngineMouse();
        if (game.IsOnPause()) game.Unpause();
    }

    JournalMouse ReadMouse(const JournalInput& events) {
        listScrollBar.reset();
        hintScrollBar.reset();
        cursorClient.reset();
        const auto window = OQIMouse::WindowRoot(hWndApp);
        const bool focused = OQIMouse::HasFocus(window);
        if (mouseEvents.TakeFocusLoss() || !focused) {
            if (std::exchange(mouseHadFocus, false)) RestoreEngineMouse();
            mouseDown = false;
            mouseEvents.Discard();
            listGrabOffset.reset();
            hintGrabOffset.reset();
        }
        if (!focused) return {};

        RECT client{};
        if (!OQIMouse::ClientBounds(window, client)) return {};
        cursorClient = client;

        const bool regainedFocus = !std::exchange(mouseHadFocus, true);
        if (regainedFocus) {
            if (mouseEvents.IsListening() || mouseEvents.Start(window)) {
                restoreEngineMouse = zinput->GetDeviceEnabled(zINPUT_MOUSE) != 0;
                if (restoreEngineMouse) zinput->SetDeviceEnabled(zINPUT_MOUSE, FALSE);
                ApplyMouseAcquisition();
                systemPointer.Enter(window, client, std::exchange(centerMouseOnOpen, false));
            }
        }

        const auto input = mouseEvents.Take();
        POINT desktop{};
        if (!GetCursorPos(&desktop)) {
            if (!input.position) return {};
            desktop = *input.position;
        }
        const auto cursor = CursorPixels(client, desktop);
        if (!cursor) return {};

        const bool down = input.down || WindowsKey(VK_LBUTTON)
            || (!mouseEvents.IsListening() && zinput->GetMouseButtonPressedLeft() != 0);
        const bool wasDown = std::exchange(mouseDown, down);
        const bool pressed = !regainedFocus && (input.pressed || (!wasDown && (down || events.mouseClick)));
        if (!down || regainedFocus) {
            listGrabOffset.reset();
            hintGrabOffset.reset();
        }

        cursorX = cursor->x;
        cursorY = cursor->y;

        int panelX, panelY;
        GetPixelPos(panelX, panelY);
        const int wheel = regainedFocus ? 0 : (input.wheel ? input.wheel : events.mouseWheel);
        if (pressed) ++mouseClicks;
        mouseWheelSteps += static_cast<unsigned>(wheel < 0 ? -wheel : wheel);
        return {anx(cursorX - panelX), any(cursorY - panelY), wheel, true, pressed};
    }

    std::string MouseDiagnostics() const {
        char label[96];
        std::snprintf(label, sizeof(label), "%s  %s  X%d Y%d  P%u C%u W%u", version,
                      mouseEvents.IsListening() ? "WIN" : "ZEN", cursorX, cursorY,
                      mouseEvents.Count(), mouseClicks, mouseWheelSteps);
        return label;
    }

    JournalInput TakeInput() {
        auto input = std::exchange(pending, JournalInput{});
        constexpr int keys[] = {VK_ESCAPE, VK_RETURN, VK_UP, VK_DOWN, VK_PRIOR, VK_NEXT, VK_LEFT, VK_RIGHT};
        const bool foreground = ForegroundGameWindow() != nullptr;
        bool pressed[std::size(keys)]{};
        for (std::size_t i = 0; i < std::size(keys); ++i) {
            const bool down = foreground && WindowsKey(keys[i]);
            pressed[i] = down && !std::exchange(windowsKeys[i], down);
        }
        input.close = input.close || pressed[0];
        input.hint = input.hint || pressed[1];
        const int vertical = static_cast<int>(pressed[3]) - static_cast<int>(pressed[2]);
        const int pages = static_cast<int>(pressed[5]) - static_cast<int>(pressed[4]);
        const int horizontal = static_cast<int>(pressed[7]) - static_cast<int>(pressed[6]);
        if (foreground && WindowsKey(VK_SHIFT)) {
            if (!input.hintLines && !input.selection) input.hintLines = vertical;
            if (!input.hintPages && !input.pages) input.hintPages = pages;
        } else {
            if (!input.selection && !input.hintLines) input.selection = vertical;
            if (!input.pages && !input.hintPages) input.pages = pages;
        }
        if (!input.locations) input.locations = horizontal;
        return input;
    }

    void Blit() override {
        const auto previousSource = zrenderer->GetAlphaBlendSource();
        zrenderer->SetAlphaBlendSource(zRND_ALPHA_SOURCE_MATERIAL);
        zCView::Blit();
        for (const auto& rule : rules) DrawRule(rule.bounds, 0, rule.color);
        for (const auto* bar : {&listScrollBar, &hintScrollBar}) {
            if (*bar) {
                const int width = nax((*bar)->track.width);
                DrawRule((*bar)->track, (std::max)(0, (width - 2) / 2), lineInk);
                DrawRule((*bar)->thumb, (std::max)(0, (width - 4) / 2),
                         (*bar)->highlighted ? 0xFFFFEAC4 : bronzeInk);
            }
        }
        if (cursorClient && OQIMouse::HasFocus(hWndApp)) {
            CURSORINFO systemCursor{};
            systemCursor.cbSize = sizeof(systemCursor);
            const bool visible = GetCursorInfo(&systemCursor)
                && (systemCursor.flags & CURSOR_SHOWING) && systemCursor.hCursor;
            POINT desktop{};
            if (!visible && GetCursorPos(&desktop)) {
                if (const auto cursor = CursorPixels(*cursorClient, desktop)) {
                    const auto size = std::clamp(screen->psizey / 42, 16, 48);
                    DrawSprite(cursorView, cursor->x, cursor->y, size, size, 0xFFFFFFFF);
                }
            }
        }
        zrenderer->SetAlphaBlendSource(previousSource);
    }

    int HandleEvent(int key) override {
        switch (key) {
        case MOUSE_WHEELUP: ++pending.mouseWheel; return TRUE;
        case MOUSE_WHEELDOWN: --pending.mouseWheel; return TRUE;
        case MOUSE_BUTTONLEFT: pending.mouseClick = true; return TRUE;
        case KEY_ESCAPE: pending.close = true; return TRUE;
        case KEY_UP:
            if (ShiftDown()) --pending.hintLines;
            else --pending.selection;
            return TRUE;
        case KEY_DOWN:
            if (ShiftDown()) ++pending.hintLines;
            else ++pending.selection;
            return TRUE;
        case KEY_PGUP:
            if (ShiftDown()) --pending.hintPages;
            else --pending.pages;
            return TRUE;
        case KEY_PGDN:
            if (ShiftDown()) ++pending.hintPages;
            else ++pending.pages;
            return TRUE;
        case KEY_LEFT: --pending.locations; return TRUE;
        case KEY_RIGHT: ++pending.locations; return TRUE;
        case KEY_RETURN:
        case KEY_NUMPADENTER: pending.hint = true; return TRUE;
        default: return TRUE;
        }
    }
};

static JournalView* journal = nullptr;
static int hintOffset = 0;
static int firstVisible = 0;

static void HideHint() {
    revealedQuest.reset();
    hintOffset = 0;
    if (journal) journal->hintGrabOffset.reset();
}

static void CloseJournal() {
    HideHint();
    delete std::exchange(journal, nullptr);
}

struct JournalLayout {
    int width, height, line, margin, top, split, listRight, details, footer, rows;

    JournalLayout() {
        width = journal->anx(journal->psizex);
        height = journal->any(journal->psizey);
        const int padding = std::clamp(screen->psizey / 50, 12, 26);
        margin = (std::max)(1, journal->anx(padding));
        top = journal->any(padding);
        line = (std::max)(1, journal->any(journal->GetFont()->GetFontY() + 4));
        split = width * 53 / 100;
        listRight = split - margin;
        details = split + margin;
        footer = height - top - 2 * line;
        rows = (std::max)(0, (footer - Y(5) - line / 3) / line);
    }

    int Y(int row) const { return top + row * line; }
    int Right() const { return width - margin; }
};

enum class Ink : unsigned long {
    Paper = 0xFFF0DFBB,
    Bright = 0xFFFFEAC4,
    Muted = 0xFFD6C6A5,
    Gold = bronzeInk,
    Done = 0xFFDCEAB9,
    Missed = 0xFFFFC2A7,
};

static void Rule(int x, int y, int width, int height, unsigned long color = lineInk) {
    journal->rules.push_back({{x, y, width, height}, color});
}

static void HorizontalRule(int left, int right, int y, unsigned long color = lineInk, int pixels = 1) {
    Rule(left, y, right - left, (std::max)(1, journal->any(pixels)), color);
}

static void ReplaceYoForJournalFont(std::string& text) {
    // The Russian journal font lacks the Windows-1251 glyphs for Ё and ё.
    std::replace(text.begin(), text.end(), '\xA8', '\xC5');
    std::replace(text.begin(), text.end(), '\xB8', '\xE5');
}

static int TextWidth(const std::string& text) {
    return journal->anx(journal->GetFont()->GetFontX(zSTRING(text.c_str())));
}

static std::string FitText(std::string text, int width) {
    if (width <= 0) return {};
    ReplaceYoForJournalFont(text);
    if (TextWidth(text) <= width) return text;
    const int ellipsis = TextWidth("...");
    if (ellipsis > width) return {};
    while (!text.empty() && TextWidth(text) + ellipsis > width) text.pop_back();
    return text + "...";
}

static void Text(int x, int y, const std::string& text, int width, Ink ink = Ink::Paper) {
    const auto fitted = FitText(text, width);
    if (fitted.empty()) return;
    const zSTRING line(fitted.c_str());
    zCOLOR color;
    color.dword = static_cast<unsigned long>(ink);
    journal->CreateText(x, y, line, 0.0f, color, 0, 1);
    // The engine draws the most recently added text first.
    zCOLOR shadow;
    shadow.dword = 0xFF0A0907;
    journal->CreateText(x + journal->anx(1), y + journal->any(1), line, 0.0f, shadow, 0, 1);
}

static std::vector<std::string> WrapText(std::string text, int width) {
    ReplaceYoForJournalFont(text);
    std::vector<std::string> lines;
    std::string line;
    for (std::size_t start = 0; start < text.size();) {
        auto end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        const auto word = text.substr(start, end - start);
        auto candidate = line.empty() ? word : line + ' ' + word;
        if (!line.empty() && TextWidth(candidate) > width) {
            lines.push_back(std::move(line));
            line = word;
        } else {
            line = std::move(candidate);
        }
        start = end + 1;
    }
    if (!line.empty()) lines.push_back(std::move(line));
    return lines;
}

static std::string Russian(std::wstring_view value) {
    if (value.empty()) return {};
    const int length = static_cast<int>(value.size());
    const int bytes = WideCharToMultiByte(1251, 0, value.data(), length, nullptr, 0, nullptr, nullptr);
    std::string text(bytes, '\0');
    if (bytes) WideCharToMultiByte(1251, 0, value.data(), length, text.data(), bytes, nullptr, nullptr);
    return text;
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

static const wchar_t* ClosedNotice(std::size_t id) {
    if (SelectedLocation() != Location::Kondehra) return SelectedCatalog().entries[id].caution;
    if (id == Fortress) return L"Знакомство с Сандроком закрыло задание у моста.";
    if (id == Testament) return L"Началось поручение в Варанте. Завещание уже закрыто.";
    return L"Отказ в диалоге закрыл поручение.";
}

static constexpr int questFilterCount = 7;
static constexpr int rawFilterCount = 5;
static constexpr int rawPageSize = 13;
static constexpr int wheelLines = 3;

static const wchar_t* DepartureLabel(Step step) {
    switch (step) {
    case Step::Open: return L"Осталось";
    case Step::Missed: return L"Пропущено";
    case Step::Unknown: return L"Нужно проверить";
    case Step::Done: return L"Этап завершён";
    default: return L"Другой этап";
    }
}

static constexpr Step departureFilters[] = {Step::Open, Step::Missed, Step::Unknown};

static constexpr Location tabs[] = {Location::Kondehra, Location::Albatros, Location::Tamora};

static int SelectedTab() {
    return static_cast<int>(std::find(std::begin(tabs), std::end(tabs), SelectedLocation()) - std::begin(tabs));
}

static JournalRect TabBounds(const JournalLayout& ui, int tab) {
    const int width = (ui.Right() - ui.margin) / static_cast<int>(std::size(tabs));
    return {ui.margin + tab * width, ui.Y(1), width, ui.line};
}

static void PaintTabs(const JournalLayout& ui, const JournalMouse& mouse) {
    HorizontalRule(ui.margin, ui.Right(), ui.Y(2) - journal->any(2));
    for (int i = 0; i < static_cast<int>(std::size(tabs)); ++i) {
        const auto name = Russian(catalogs[static_cast<std::size_t>(tabs[i])].name);
        const bool active = tabs[i] == SelectedLocation();
        const auto cell = TabBounds(ui, i);
        const int labelX = cell.x + (std::max)(0, (cell.width - TextWidth(name)) / 2);
        Text(labelX, cell.y, name, cell.x + cell.width - labelX,
             active ? Ink::Bright : mouse.Over(cell) ? Ink::Bright : Ink::Gold);
        if (active) {
            HorizontalRule(cell.x + ui.margin / 3, cell.x + cell.width - ui.margin / 3,
                           cell.y + ui.line - journal->any(2), bronzeInk, 3);
        }
    }
}

static void PaintProgress(const JournalLayout& ui) {
    std::string line;
    if (!currentVisit) {
        line = Russian(L"Прогресс визита доступен в текущей локации второй главы.");
    } else if (const auto percent = progress.Percent()) {
        line = Russian(L"До отплытия ") + std::to_string(*percent) + "% ("
            + std::to_string(progress.completed) + "/" + std::to_string(progress.Total()) + ")"
            + Russian(L"  ·  Осталось ") + std::to_string(progress.remaining);
    } else {
        line = Russian(L"Нет подтверждённых дел для расчёта прогресса.");
    }
    if (currentVisit) {
        line += Russian(L"  ·  Пропущено ") + std::to_string(progress.missed);
        line += Russian(L"  ·  Неясно ") + std::to_string(progress.unknown);
        line += Russian(L"  ·  Другой этап ") + std::to_string(progress.deferred);
    }
    Text(ui.margin, ui.Y(2) + journal->any(3), line, ui.Right() - ui.margin, Ink::Paper);
}

static std::string QuestTitle(const QuestItem& item) {
    return item.title.empty() ? Russian(L"Название отсутствует") : item.title;
}

static const wchar_t* QuestStatus(const QuestItem& item) {
    if (beforeDeparture) return DepartureLabel(item.step);
    const auto status = item.result.status;
    if (currentVisit && status != Status::Done && status != Status::Missed) {
        if (item.step == Step::Deferred) return L"Другой этап";
        if (item.step == Step::Done) return L"Этап завершён";
        if (item.step == Step::Open && status == Status::Later) return L"Откроется здесь";
        if (item.step == Step::Open && status == Status::Unknown) return L"Местный этап";
    }
    return StatusText(status);
}

static std::optional<JournalView::ScrollBar> UpdateScrollBar(
        const JournalRect& track, int count, int capacity, int& offset,
        std::optional<int>& grab, const JournalMouse& mouse) {
    const int maxOffset = (std::max)(0, count - capacity);
    offset = std::clamp(offset, 0, maxOffset);
    if (!maxOffset || track.height < 2) {
        grab.reset();
        return std::nullopt;
    }
    const int thumbHeight = std::clamp(track.height * capacity / count,
                                       (std::max)(1, track.height / (capacity * 2)), track.height - 1);
    const int travel = track.height - thumbHeight;
    const int thumbY = track.y + (travel * offset + maxOffset / 2) / maxOffset;
    const bool hovered = mouse.Over(track);
    if (mouse.pressed && hovered) {
        grab = mouse.y >= thumbY && mouse.y < thumbY + thumbHeight
            ? mouse.y - thumbY : thumbHeight / 2;
    }
    if (grab) {
        const int position = std::clamp(mouse.y - track.y - *grab, 0, travel);
        offset = (position * maxOffset + travel / 2) / travel;
    }
    return JournalView::ScrollBar{track,
        {track.x, track.y + (travel * offset + maxOffset / 2) / maxOffset, track.width, thumbHeight},
        hovered || grab.has_value()};
}

static Ink QuestInk(const QuestItem& item);

static void PaintDetails(const JournalLayout& ui, std::size_t id, const JournalInput& input,
                         const JournalMouse& mouse) {
    const auto& item = questItems[id];
    const auto& quest = SelectedCatalog().entries[id];
    const int width = ui.Right() - ui.details;
    Text(ui.details, ui.Y(4), Russian(L"Выбранное задание"), width, Ink::Gold);

    int row = 5;
    for (const auto& line : WrapText(QuestTitle(item), width)) {
        if (row >= 5 + ui.rows) return;
        Text(ui.details, ui.Y(row++), line, width, Ink::Bright);
    }
    if (row >= 5 + ui.rows) return;
    Text(ui.details, ui.Y(row++), Russian(QuestStatus(item)), width, QuestInk(item));
    if (row >= 5 + ui.rows) return;

    const auto button = [](bool open) {
        return Russian(open ? L"Скрыть подсказку" : L"Показать подсказку") + "   Enter";
    };
    const JournalRect buttonBounds{ui.details, ui.Y(row), width, ui.line};
    if (input.hint || (mouse.pressed && mouse.Over(buttonBounds))) {
        if (revealedQuest) HideHint();
        else revealedQuest = id;
    }
    Text(ui.details, ui.Y(row++), button(revealedQuest.has_value()), width,
         mouse.Over(buttonBounds) ? Ink::Bright : Ink::Gold);
    if (!revealedQuest || row >= 5 + ui.rows) return;

    const int barWidth = journal->anx(10);
    const int textWidth = width - barWidth - journal->anx(7);
    std::vector<std::string> lines;
    const auto append = [&](const std::string& paragraph) {
        auto wrapped = WrapText(paragraph, textWidth);
        lines.insert(lines.end(), std::make_move_iterator(wrapped.begin()), std::make_move_iterator(wrapped.end()));
    };
    std::wstring_view walkthrough = quest.walkthrough;
    for (int step = 1; !walkthrough.empty(); ++step) {
        const auto end = walkthrough.find(L'\n');
        append(std::to_string(step) + ". " + Russian(walkthrough.substr(0, end)));
        if (end == std::wstring_view::npos) break;
        walkthrough.remove_prefix(end + 1);
    }
    if (item.result.evidence == Evidence::ClosedRule) {
        if (const auto* notice = ClosedNotice(id)) append(Russian(notice));
    }
    const int available = 5 + ui.rows - row;
    const int count = static_cast<int>(lines.size());
    const int capacity = count > available && available > 1 ? available - 1 : available;
    const JournalRect details{ui.details, ui.Y(4), width, ui.footer - ui.Y(4)};
    const int wheel = mouse.Over(details) ? mouse.wheel * wheelLines : 0;
    hintOffset += input.hintLines + input.hintPages * capacity - wheel;
    const JournalRect track{ui.Right() - barWidth, ui.Y(row), barWidth, ui.line * capacity};
    journal->hintScrollBar = UpdateScrollBar(track, count, capacity, hintOffset,
                                            journal->hintGrabOffset, mouse);
    const int end = (std::min)(hintOffset + capacity, count);
    for (int i = hintOffset; i < end; ++i) {
        Text(ui.details, ui.Y(row++), lines[i], textWidth);
    }
    if (capacity < available) {
        const auto position = std::to_string(hintOffset + 1) + "-" + std::to_string(end)
            + "/" + std::to_string(count);
        Text(ui.details, ui.Y(row), "Shift+PgUp/PgDn  " + position, width, Ink::Muted);
    }
}

static Ink QuestInk(const QuestItem& item) {
    if (item.result.status == Status::Done || (currentVisit && item.step == Step::Done)) return Ink::Done;
    if (item.result.status == Status::Missed || (currentVisit && item.step == Step::Missed)) return Ink::Missed;
    return Ink::Paper;
}

static void PaintQuests(const JournalLayout& ui, const JournalInput& input, const JournalMouse& mouse) {
    const auto& catalog = SelectedCatalog();
    Text(ui.margin, ui.Y(0), Russian(L"Журнал экспедиции · Глава 2"), ui.Right() - ui.margin, Ink::Gold);
    PaintTabs(ui, mouse);
    Text(ui.margin, ui.footer, Russian(L"Стрелки   Выбор и локация     PgUp/PgDn   Страница"),
         ui.Right() - ui.margin, Ink::Muted);
    Text(ui.margin, ui.footer + ui.line, Russian(L"Shift+F   Фильтр     Shift+V   Перед отплытием     Shift+D   Флаги"),
         ui.Right() - ui.margin - TextWidth(version) - ui.margin, Ink::Muted);
    if (!verifiedBuild) {
        Text(ui.margin, ui.Y(2), Russian(L"Скрипты русской версии 2.7.5.2 не распознаны."), ui.Right() - ui.margin);
        return;
    }

    PaintProgress(ui);
    const wchar_t* filter = L"Все";
    if (questFilter != 0) {
        filter = beforeDeparture ? DepartureLabel(departureFilters[questFilter - 1])
                                 : StatusText(static_cast<Status>(questFilter - 1));
    }
    const auto mode = Russian(beforeDeparture ? L"Перед отплытием" : L"Все задания");
    Text(ui.margin, ui.Y(3), mode + Russian(L"  ·  Фильтр: ") + Russian(filter), ui.Right() - ui.margin);
    if (beforeDeparture && !currentVisit) return;
    if (ui.rows == 0) return;
    HorizontalRule(ui.margin, ui.Right(), ui.Y(4) - journal->any(3));
    Rule(ui.split, ui.Y(4), (std::max)(1, journal->anx(1)), ui.footer - ui.Y(4) - ui.line / 3);

    std::vector<std::size_t> visible;
    for (std::size_t i = 0; i < questItems.size(); ++i) {
        const auto& item = questItems[i];
        if (beforeDeparture) {
            if (!OQIDeparture::NeedsReview(item.step)) continue;
            if (questFilter != 0 && item.step != departureFilters[questFilter - 1]) continue;
        } else if (questFilter != 0 && static_cast<int>(item.result.status) + 1 != questFilter) {
            continue;
        }
        visible.push_back(i);
    }
    if (visible.empty()) {
        HideHint();
        journal->listGrabOffset.reset();
        Text(ui.margin, ui.Y(5), Russian(L"Для этого фильтра заданий нет."), ui.Right() - ui.margin);
        return;
    }

    const int count = static_cast<int>(visible.size());
    const int maxFirst = (std::max)(0, count - ui.rows);
    firstVisible = std::clamp(firstVisible, 0, maxFirst);
    const JournalRect list{ui.margin, ui.Y(5), ui.listRight - ui.margin, ui.rows * ui.line};
    const int rowUnderMouse = mouse.Over(list) ? firstVisible + (mouse.y - list.y) / ui.line : count;
    const bool clickedRow = mouse.pressed && rowUnderMouse < count;
    const int keyboardMove = input.selection + input.pages * ui.rows;
    selected = std::clamp(clickedRow ? rowUnderMouse : selected + keyboardMove, 0, count - 1);
    const auto selectedId = visible[selected];
    if (revealedQuest != selectedId) HideHint();

    if (keyboardMove && !clickedRow) {
        firstVisible = std::clamp(firstVisible, (std::max)(0, selected - ui.rows + 1), selected);
    }
    const int barWidth = journal->anx(10);
    const JournalRect track{ui.split - barWidth - journal->anx(4), list.y, barWidth, list.height};
    const JournalRect listColumn{list.x, ui.Y(4), ui.details - list.x, ui.footer - ui.Y(4)};
    if (mouse.Over(listColumn)) firstVisible -= mouse.wheel * wheelLines;
    journal->listScrollBar = UpdateScrollBar(track, count, ui.rows, firstVisible,
                                            journal->listGrabOffset, mouse);
    const int end = (std::min)(firstVisible + ui.rows, count);
    const auto indicator = std::to_string(firstVisible + 1) + "-" + std::to_string(end)
        + " / " + std::to_string(count);
    int statusColumn = TextWidth(Russian(L"Статус"));
    for (const auto id : visible) {
        statusColumn = (std::max)(statusColumn, TextWidth(Russian(QuestStatus(questItems[id]))));
    }
    statusColumn = (std::min)(statusColumn, list.width * 46 / 100);
    const int statusHeading = ui.listRight - TextWidth(Russian(L"Статус"));
    Text(ui.margin, ui.Y(4), Russian(L"Задания   ") + indicator, statusHeading - ui.margin, Ink::Gold);
    Text(statusHeading, ui.Y(4), Russian(L"Статус"), ui.listRight - statusHeading, Ink::Gold);
    for (int i = firstVisible; i < end; ++i) {
        const auto id = visible[i];
        const auto& item = questItems[id];
        std::string title = QuestTitle(item);
        if (catalog.entries[id].continuesBeyondIsland) title += " *";
        const auto status = Russian(QuestStatus(item));
        const int y = ui.Y(5 + i - firstVisible);
        const auto fittedStatus = FitText(status, statusColumn);
        const int statusX = ui.listRight - TextWidth(fittedStatus);
        const bool hovered = mouse.Over({list.x, y, list.width, ui.line});
        if (i == selected) {
            Rule(ui.margin - journal->anx(8), y + journal->any(3), (std::max)(1, journal->anx(2)),
                 ui.line - journal->any(7), bronzeInk);
        }
        Text(ui.margin, y, title, list.width - statusColumn - ui.margin / 2,
             i == selected || hovered ? Ink::Bright : Ink::Paper);
        Text(statusX, y, fittedStatus, ui.listRight - statusX, QuestInk(item));
    }
    PaintDetails(ui, selectedId, input, mouse);
}

static void PaintRaw(const JournalLayout& ui) {
    static constexpr const char* labels[] = {"ALL", "VALUE 1", "VALUE 2", "VALUE 0", "OTHER"};
    std::vector<const RawItem*> visible;
    for (const auto& item : rawItems) {
        const int match = item.value == 1 ? 1 : item.value == 2 ? 2 : item.value == 0 ? 3 : 4;
        if (!rawFilter || rawFilter == match) visible.push_back(&item);
    }
    const int count = static_cast<int>(visible.size());
    const int pageSize = std::clamp(ui.rows, 1, rawPageSize);
    const int pages = (std::max)(1, (count + pageSize - 1) / pageSize);
    page = std::clamp(page, 0, pages - 1);
    const int width = ui.Right() - ui.margin;
    Text(ui.margin, ui.Y(0), Russian(L"Журнал экспедиции · Флаги"), width, Ink::Gold);
    char line[256];
    std::snprintf(line, sizeof(line), "%s | %d/%d | RAW FLAGS %d/%d", labels[rawFilter], page + 1, pages,
                  static_cast<int>(rawItems.size()), OQIManifest::kCount);
    Text(ui.margin, ui.Y(2), line, width, Ink::Muted);
    Text(ui.margin, ui.footer, Russian(L"Влево/вправо · Страница    Shift+F · Фильтр    R · Обновить"), width, Ink::Muted);
    Text(ui.margin, ui.footer + ui.line, Russian(L"Esc · Закрыть    Shift+D · Задания"), width, Ink::Muted);
    if (ui.rows == 0) return;
    const int begin = page * pageSize;
    const int end = (std::min)(begin + pageSize, count);
    for (int i = begin; i < end; ++i) {
        std::snprintf(line, sizeof(line), "[%d] %s", visible[i]->value, visible[i]->name.c_str());
        Text(ui.margin, ui.Y(5 + i - begin), line, width);
    }
    if (!count) Text(ui.margin, ui.Y(5), Russian(L"Для этого фильтра флагов нет."), width);
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
static Key hintKey{KEY_H}, locationKey{KEY_T};

static void ResetQuestSelection() {
    selected = firstVisible = 0;
    if (journal) journal->listGrabOffset.reset();
    HideHint();
}

static bool JournalTogglePressed() {
    const bool focused = OQIMouse::HasFocus(hWndApp);
    const auto virtualKey = MapVirtualKeyW(KEY_APOSTROPHE, MAPVK_VSC_TO_VK_EX);
    const bool down = focused && (zinput->KeyPressed(KEY_APOSTROPHE) != 0
        || WindowsKey(virtualKey ? static_cast<int>(virtualKey) : VK_OEM_7));
    const bool wasDown = std::exchange(apostropheWasDown, down);
    return down && !wasDown;
}

static void Loop() {
    if (loading || !zinput || !player || !screen || !parser || !ogame || !ogame->GetGameWorld()) return;

    const bool shift = ShiftDown();
    const bool filterPressed = filterKey.Pressed() && shift;
    const bool departurePressed = departureKey.Pressed() && shift;
    const bool diagnosticPressed = diagnosticKey.Pressed() && shift;
    const bool hintPressed = hintKey.Pressed() && shift;
    const bool locationPressed = locationKey.Pressed() && shift;
    const bool toggle = JournalTogglePressed();
    if (player->inventory2.IsOpen()
        || !oCInformationManager::GetInformationManager().HasFinished()) {
        CloseJournal();
        return;
    }

    if (journal && toggle) {
        CloseJournal();
        return;
    }
    if (!journal) {
        if (!toggle || ogame->IsOnPause() || !screen->GetFont()
            || screen->psizex <= 0 || screen->psizey <= 0) return;
        journal = new JournalView(*ogame);
        if (!location) location = CurrentLocation().value_or(Location::Kondehra);
        if (diagnostics) RefreshRaw();
        else RefreshQuests();
    }

    auto input = journal->TakeInput();
    if (input.close) {
        CloseJournal();
        return;
    }
    input.hint = input.hint || hintPressed;
    if (diagnosticPressed) {
        diagnostics = !diagnostics;
        journal->listGrabOffset.reset();
        HideHint();
        input = {};
        if (diagnostics) RefreshRaw();
        else RefreshQuests();
    }

    const bool refreshPressed = zinput->KeyToggled(KEY_R);
    journal->ClrPrintwin();
    journal->rules.clear();
    const JournalLayout ui;
    HorizontalRule(ui.margin, ui.Right(), ui.footer - ui.line / 3);
    const auto mouse = journal->ReadMouse(input);
    const std::string status = diagnostics ? journal->MouseDiagnostics() : version;
    const int versionWidth = TextWidth(status);
    Text(ui.Right() - versionWidth, ui.footer + ui.line, status, versionWidth, Ink::Muted);
    const std::string closeLabel = "Esc";
    const int closeWidth = TextWidth(closeLabel);
    const JournalRect closeButton{ui.Right() - closeWidth - ui.margin, ui.Y(0),
                                  closeWidth + ui.margin, ui.line};
    const bool closeHovered = mouse.Over(closeButton);
    if (mouse.pressed && closeHovered) {
        CloseJournal();
        return;
    }
    Text(ui.Right() - closeWidth, closeButton.y, closeLabel, closeWidth,
         closeHovered ? Ink::Bright : Ink::Gold);
    if (diagnostics) {
        if (filterPressed) { rawFilter = (rawFilter + 1) % rawFilterCount; page = 0; }
        if (refreshPressed) RefreshRaw();
        page += input.locations + input.pages;
        if (mouse.Over({ui.margin, ui.Y(5), ui.Right() - ui.margin, ui.rows * ui.line})) page -= mouse.wheel;
        PaintRaw(ui);
        return;
    }

    const int tabCount = static_cast<int>(std::size(tabs));
    const int currentTab = SelectedTab();
    const int movement = input.locations + static_cast<int>(locationPressed);
    int nextTab = (currentTab + movement % tabCount + tabCount) % tabCount;
    if (mouse.pressed) {
        for (int tab = 0; tab < tabCount; ++tab) {
            if (mouse.Over(TabBounds(ui, tab))) nextTab = tab;
        }
    }
    if (nextTab != currentTab) {
        location = tabs[nextTab];
        questFilter = 0;
        ResetQuestSelection();
        input = {};
        RefreshQuests();
    }
    if (departurePressed) {
        beforeDeparture = !beforeDeparture;
        questFilter = 0;
        ResetQuestSelection();
        input.hint = false;
    }
    if (filterPressed) {
        const int count = beforeDeparture ? 1 + static_cast<int>(std::size(departureFilters)) : questFilterCount;
        questFilter = (questFilter + 1) % count;
        ResetQuestSelection();
        input.hint = false;
    }
    if (refreshPressed || std::chrono::steady_clock::now() >= nextRefresh) RefreshQuests();
    PaintQuests(ui, input, mouse);
}

class LoadScope {
    bool wasLoading = loading;
public:
    LoadScope() {
        loading = true;
        updatedFromInput = false;
        CloseJournal();
        verifiedBuild = false;
        beforeDeparture = false;
        diagnostics = false;
        questFilter = rawFilter = page = selected = firstVisible = 0;
        rawItems.clear();
        questItems.clear();
        currentVisit = false;
        progress = {};
        location.reset();
    }
    ~LoadScope() { loading = wasLoading; }
};

}  // namespace OQI

// The stock game loop polls this entry point before rendering, even during a pause.
void __cdecl OQI_GetInput();
auto OQI_GetInputHook = Union::CreateHook(
    reinterpret_cast<void*>(0x007A55C0), &OQI_GetInput, Union::HookType::Hook_Detours);

void __cdecl OQI_GetInput() {
    OQI_GetInputHook();
    OQI::Loop();
    OQI::updatedFromInput = true;
}

// Keep the render path for renderers that bypass the stock input poll.
void __fastcall OQI_Render(oCGame*, void*);
auto OQI_RenderHook = Union::CreateHook(
    SIGNATURE_OF(&oCGame::Render), &OQI_Render, Union::HookType::Hook_Detours);

void __fastcall OQI_Render(oCGame* self, void* vtable) {
    if (!std::exchange(OQI::updatedFromInput, false)) OQI::Loop();
    OQI_RenderHook(self, vtable);
}

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

void __fastcall OQI_Done(oCGame*, void*);
auto OQI_DoneHook = Union::CreateHook(
    SIGNATURE_OF(&oCGame::Done), &OQI_Done, Union::HookType::Hook_Detours);

void __fastcall OQI_Done(oCGame* self, void* vtable) {
    OQI::LoadScope load;
    OQI_DoneHook(self, vtable);
}

}  // namespace GOTHIC_NAMESPACE
