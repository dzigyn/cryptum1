#pragma once

#include <vector>

#include "library_loader.h"

namespace menu {

// Главный цикл меню; возвращает управление при выборе пункта «Выход»
void run(const std::vector<library_loader::CipherLibrary>& libraries);

}  // namespace menu
