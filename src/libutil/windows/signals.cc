#include "nix/util/signals.hh"

namespace nix {

std::unique_ptr<InterruptCallback> createInterruptCallback(fun<void()> callback)
{
    // Does nothing on Windows — no POSIX signals.
    return std::make_unique<InterruptCallback>();
}

} // namespace nix
