// Union G2A read-only quest-flag viewer. NO Ninja, no script changes.
#pragma once
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "quest_manifest.hpp"

namespace GOTHIC_NAMESPACE {
namespace OQI {
struct Item { std::string name; int state; };
static std::vector<Item> items;
static bool opened = false;
static int filter = 0, page = 0;
static float lastFrame = -1.0f;
static constexpr int pageSize = 13;
static const char* filterNames[] = { "ALL", "ACTIVE(1)", "DONE(2)", "ZERO(0)", "OTHER" };
static bool Matches(int v) {
    switch (filter) {
    case 1: return v == 1; case 2: return v == 2;
    case 3: return v == 0; case 4: return v != 0 && v != 1 && v != 2;
    default: return true;
    }
}
static void Refresh() {
    items.clear(); page = 0;
    if (!parser) return;
    for (int i = 0; i < OQIManifest::kCount; ++i) {
        auto name = OQIManifest::kNames[i];
        zCPar_Symbol* s = parser->GetSymbol(name);
        if (!s) continue;
        int value = 0;
        s->GetValue(value, 0); // Read-only!
        items.push_back({ name, value });
    }
}
static void Text(int row, const char* str) {
    if (screen) screen->Print(260, 420 + row * 280, zSTRING(str));
}
static void Paint() {
    std::vector<const Item*> filtered;
    int done = 0, active = 0, zero = 0, other = 0;
    for (auto& item : items) {
        if (item.state == 2) ++done;
        else if (item.state == 1) ++active;
        else if (item.state == 0) ++zero;
        else ++other;
        if (Matches(item.state)) filtered.push_back(&item);
    }
    int pages = (std::max)(1, int((filtered.size() + pageSize - 1) / pageSize));
    page = std::clamp(page, 0, pages - 1);
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "ODYSSEE QUEST INSPECTOR | %s | %d/%d", filterNames[filter], page + 1, pages);
    Text(0, buffer);
    std::snprintf(buffer, sizeof(buffer), "FLAGS %d/%d | 0:%d 1:%d 2:%d OTHER:%d",
        int(items.size()), OQIManifest::kCount, zero, active, done, other);
    Text(1, buffer);
    Text(2, "APOSTROPHE: CLOSE | F6: FILTER | LEFT/RIGHT: PAGE | R: REFRESH");
    Text(3, "RAW SCRIPT FLAGS -- NOT JOURNAL QUEST COUNTS");
    int begin = page * pageSize, end = (std::min)(begin + pageSize, int(filtered.size()));
    for (int i = begin; i < end; ++i) {
        std::snprintf(buffer, sizeof(buffer), "[%d] %s", filtered[i]->state, filtered[i]->name.c_str());
        Text(i - begin + 4, buffer);
    }
    if (filtered.empty()) Text(4, "NO MATCHING FLAGS");
}
static void Loop() {
    if (!zinput || !player || !screen || !parser || !ztimer) return;
    float frame = ztimer->totalTimeFloatSecs;
    if (frame == lastFrame) return;
    lastFrame = frame;
    if (zinput->KeyToggled(KEY_APOSTROPHE)) {
        opened = !opened;
        if (opened) Refresh();
    }
    if (!opened) return;
    if (zinput->KeyToggled(KEY_F6)) { filter = (filter + 1) % 5; page = 0; }
    if (zinput->KeyToggled(KEY_R)) Refresh();
    if (zinput->KeyToggled(KEY_LEFT)) page = (std::max)(0, page - 1);
    if (zinput->KeyToggled(KEY_RIGHT)) ++page;
    Paint();
}
static void Reset() { items.clear(); opened = false; lastFrame = -1.0f; }
}
void Game_EntryPoint() {}
void Game_Init() {}
void Game_Exit() { OQI::Reset(); }
void Game_PreLoop() {}
void Game_Loop() { OQI::Loop(); }
void Game_PostLoop() {}
void Game_MenuLoop() {}
void Game_SaveBegin() {}
void Game_SaveEnd() {}
void LoadBegin() { OQI::Reset(); }
void LoadEnd() {}
void Game_LoadBegin_NewGame() { LoadBegin(); }
void Game_LoadEnd_NewGame() { LoadEnd(); }
void Game_LoadBegin_SaveGame() { LoadBegin(); }
void Game_LoadEnd_SaveGame() { LoadEnd(); }
void Game_LoadBegin_ChangeLevel() { LoadBegin(); }
void Game_LoadEnd_ChangeLevel() { LoadEnd(); }
void Game_LoadBegin_TriggerChangeLevel() {}
void Game_LoadEnd_TriggerChangeLevel() {}
void Game_Pause() {}
void Game_Unpause() {}
void Game_DefineExternals() {}
void Game_ApplySettings() {}
void __fastcall OQI_Render(Union::Registers& reg);
auto OQI_Hook = Union::CreatePartialHook(
    reinterpret_cast<void*>(zSwitch(0x0, 0x0, 0x0, 0x006C87EB)), &OQI_Render);
void __fastcall OQI_Render(Union::Registers& reg) {
    (void)reg;
    Game_Loop();
}
}
