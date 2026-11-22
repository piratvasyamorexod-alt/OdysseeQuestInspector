#pragma once

#include <array>
#include <cstddef>
#include <optional>

namespace OQIKondehra {

enum QuestId : std::size_t {
    Island, Fleet, Goblins, Shrine, Couple, Wolf, Errand, Fortress,
    Library, Rats, Wreck, Staff, Ring, Provisions, Testament, QuestCount
};

enum class Status { Available, Running, Done, Later, Missed, Unknown };
enum class Journal { Absent, Running, Success, Failure, Obsolete, Other };
enum class Start { Yes, Later, Unknown };
enum class Evidence { Journal, Script, StartRule, ClosedRule, Insufficient };

struct Quest {
    const char* topic;
    const char* flag;
    const wchar_t* start;
    const wchar_t* finish;
    bool continuesBeyondIsland;
};

inline constexpr std::array<Quest, QuestCount> quests{{
    {"TOPIC_KONDEHRAMISSION", "MIS_KONDEHRAMISSION",
     L"На корабле сдайте Герноту секстант и закончите три поручения.",
     L"После осмотра затонувшего корабля доложите Герноту о разведке.", false},
    {"TOPIC_ORKFLOTTE", "MIS_ORKFLOTTE",
     L"Гернот даёт поручение вместе с разведкой Кондеры. Спросите Сандрока.",
     L"На Кондере выясните путь флота. Позже отдайте Герноту перевод письма.", true},
    {"TOPIC_PIRATENGOBLINS", "MIS_PIRATENGOBLINS",
     L"Осмотрите берег с гоблинами или прочитайте записи о пиратах.",
     L"Следуйте по следу плотов и разузнайте о гоблинах у Спенсера.", true},
    {"TOPIC_INNOSSCHREIN", "MIS_INNOSSCHREIN",
     L"Поговорите с Оскаром. Расспросите Сандрока и Гигеса.",
     L"Найдите статуэтку и восстановите алтарь. Затем доложите Олдерсону.", false},
    {"TOPIC_HEIDELINDEUNDBROSIUS", "MIS_HEIDELINDEUNDBROSIUS",
     L"В первом разговоре с Аделиндой согласитесь поговорить с Брозиусом.",
     L"Найдите книгу и принесите её Аделинде.", false},
    {"TOPIC_GRAUERWOLF", "MIS_GRAUERWOLF",
     L"Спросите Гигеса об алтаре и продолжите разговор о его проблеме.",
     L"Убейте серого волка и вернитесь к Гигесу.", false},
    {"TOPIC_GRISELDALAUFBURSCH", "MIS_GRISELDALAUFBURSCH",
     L"Поговорите с Гризельдой. В диалоге с Оскаром примите поручение.",
     L"Выполните просьбы Гризельды и отнесите суп Оскару.", false},
    {"TOPIC_SANDROCKSBURG", "MIS_SANDROCKSBURG",
     L"Подойдите к подъёмному мосту до знакомства с Сандроком.",
     L"Войдите в крепость и познакомьтесь с Сандроком.", false},
    {"TOPIC_SANDROCKLANGEWEILE", "MIS_SANDROCKLANGEWEILE",
     L"Расспросите Сандрока о крепости и продолжите разговор.",
     L"Найдите тайную библиотеку в крепости.", false},
    {"TOPIC_HAFENRATTEN", "MIS_HAFENRATTEN",
     L"После разговора о крепости спросите о делах и согласитесь помочь.",
     L"Убейте крыс в порту и доложите Сандроку.", false},
    {"TOPIC_GESUNKENESSCHIFF", "MIS_GESUNKENESSCHIFF",
     L"Верните алтарь и расскажите Олдерсону. Спросите о кораблях.",
     L"Осмотрите затонувший корабль орков и сообщите о нём Герноту.", false},
    {"TOPIC_GYGESHIRTENSTAB", "MIS_GYGESHIRTENSTAB",
     L"После восстановления алтаря сообщите Гигесу. Идите за ним к месту потери.",
     L"Согласитесь искать посох и принесите его Гигесу.", false},
    {"TOPIC_ALTESGEHEIMNIS", "MIS_ALTESGEHEIMNIS",
     L"На Кондере прочитайте первый том истории о кольце.",
     L"Следуйте по книгам. Найдите и наденьте кольцо скорости.", true},
    {"TOPIC_FASSWEIN", "MIS_FASSWEIN",
     L"Осмотрите корабль орков, попрощайтесь с Сандроком и выслушайте его.",
     L"Принесите Сандроку запрошенный провиант и бочку вина.", false},
    {"TOPIC_KOROFSKITESTAMENT", "MIS_KOROFSKITESTAMENT",
     L"Прочитайте завещание в саркофаге на Кондере.",
     L"На Острове Мёртвых найдите сокровище и прочитайте письмо.", true},
}};

struct Facts {
    std::optional<int> flag;
    Journal journal = Journal::Absent;
    Start start = Start::Unknown;
    bool windowClosed = false;
    bool staffAccepted = false;
};

struct Result {
    Status status;
    Evidence evidence;
};

inline Result Evaluate(QuestId quest, const Facts& facts) {
    // Every listed MIS symbol explicitly receives LOG_SUCCESS at a verified endpoint.
    if (facts.flag == 2 || facts.journal == Journal::Success) {
        if (facts.windowClosed || facts.journal == Journal::Failure) {
            return {Status::Unknown, Evidence::Insufficient};
        }
        return {Status::Done, facts.flag == 2 ? Evidence::Script : Evidence::Journal};
    }
    if (facts.windowClosed) {
        return {Status::Missed, Evidence::ClosedRule};
    }
    if (facts.journal == Journal::Failure || facts.journal == Journal::Obsolete
        || (facts.flag && *facts.flag != 0 && *facts.flag != 1)) {
        return {Status::Unknown, Evidence::Insufficient};
    }
    if (facts.journal == Journal::Running) {
        return {Status::Running, Evidence::Journal};
    }
    if (facts.flag == 1 && (quest != Staff || facts.staffAccepted)) {
        return {Status::Running, Evidence::Script};
    }
    if (!facts.flag || facts.journal == Journal::Other) {
        return {Status::Unknown, Evidence::Insufficient};
    }
    switch (facts.start) {
    case Start::Yes: return {Status::Available, Evidence::StartRule};
    case Start::Later: return {Status::Later, Evidence::StartRule};
    default: return {Status::Unknown, Evidence::Insufficient};
    }
}

inline bool IncludeBeforeDeparture(Status status) {
    return status != Status::Done;
}

}  // namespace OQIKondehra
