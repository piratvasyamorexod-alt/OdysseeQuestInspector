#pragma once

#include "kondehra.hpp"

namespace OQIDeparture {

enum class Step { Done, Open, Missed, Deferred, Unknown };

inline Step Whole(OQIKondehra::Status status, bool canFinishHere, bool confirmedStart = false) {
    using OQIKondehra::Status;
    if (status == Status::Done) return Step::Done;
    if (status == Status::Missed) return Step::Missed;
    if (canFinishHere && (status != Status::Unknown || confirmedStart)) return Step::Open;
    return Step::Unknown;
}

inline Step Checkpoint(std::optional<bool> done, bool canFinishHere) {
    if (!done) return Step::Unknown;
    if (*done) return Step::Done;
    return canFinishHere ? Step::Open : Step::Unknown;
}

inline bool NeedsReview(Step step) {
    return step == Step::Open || step == Step::Missed || step == Step::Unknown;
}

struct Progress {
    int completed = 0;
    int remaining = 0;
    int missed = 0;
    int unknown = 0;

    void Add(Step step) {
        switch (step) {
        case Step::Done: ++completed; break;
        case Step::Open: ++remaining; break;
        case Step::Missed: ++missed; break;
        case Step::Unknown: ++unknown; break;
        case Step::Deferred: break;
        }
    }

    int Total() const { return completed + remaining; }

    std::optional<int> Percent() const {
        if (!Total()) return std::nullopt;
        return 100 * completed / Total();
    }
};

}  // namespace OQIDeparture
