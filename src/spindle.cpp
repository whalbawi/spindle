#include "spindle.h"

#include "version.h"

namespace spindle {

std::string Spindle::get_version() {
    return "Version: " + version() + " - Commit: " + git_commit_hash();
}

} // namespace spindle
