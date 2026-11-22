#include "../src/kondehra.hpp"

#include <cstdlib>

using namespace OQIKondehra;

static void Check(bool condition) {
    if (!condition) {
        std::abort();
    }
}

int main() {
    for (std::size_t i = 0; i < quests.size(); ++i) {
        const auto id = static_cast<QuestId>(i);
        Check(Evaluate(id, {}).status == Status::Unknown);
        Check(Evaluate(id, {0}).status == Status::Unknown);
        Check(Evaluate(id, {0, Journal::Absent, Start::Yes}).status == Status::Available);
        Check(Evaluate(id, {0, Journal::Absent, Start::Later}).status == Status::Later);
        Check(Evaluate(id, {2, Journal::Running}).status == Status::Done);
        Check(Evaluate(id, {0, Journal::Success}).status == Status::Done);
        Check(Evaluate(id, {0, Journal::Running}).status == Status::Running);
        Check(Evaluate(id, {3, Journal::Failure}).status == Status::Unknown);
        Check(Evaluate(id, {4, Journal::Obsolete}).status == Status::Unknown);
    }
    Check(Evaluate(Staff, {1, Journal::Absent, Start::Later}).status == Status::Later);
    Check(Evaluate(Staff, {1, Journal::Absent, Start::Yes}).status == Status::Available);
    Check(Evaluate(Staff, {1, Journal::Absent, Start::Unknown, false, true}).status == Status::Running);
    Check(Evaluate(Fortress, {0, Journal::Absent, Start::Later, true}).status == Status::Missed);
    Check(Evaluate(Rats, {3, Journal::Absent, Start::Unknown, true}).status == Status::Missed);
    Check(Evaluate(Errand, {3, Journal::Failure, Start::Unknown, true}).status == Status::Missed);
    Check(Evaluate(Testament, {2, Journal::Failure, Start::Unknown, true}).status == Status::Unknown);
    Check(Evaluate(Wolf, {std::nullopt, Journal::Absent, Start::Yes}).status == Status::Unknown);
    Check(IncludeBeforeDeparture(Status::Missed));
    Check(IncludeBeforeDeparture(Status::Unknown));
    Check(IncludeBeforeDeparture(Status::Later));
    Check(!IncludeBeforeDeparture(Status::Done));
}
