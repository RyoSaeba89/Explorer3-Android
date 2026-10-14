// See labels.h.
#include "labels.h"

#include <atomic>

namespace {
std::atomic<int> current(labels::TOUCH);
}  // namespace

namespace labels {

Style style() {
    return static_cast<Style>(current.load());
}

void setStyle(int s) {
    if (s >= 0 && s < COUNT) {
        current = s;
    }
}

}  // namespace labels
