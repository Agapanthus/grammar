#pragma once

#include <memory>
using std::shared_ptr, std::make_shared;

#include "dictionary.hpp"

namespace parseWiki {

Dictionary parseWiki(shared_ptr<istream> &&in);

} // namespace parseWiki