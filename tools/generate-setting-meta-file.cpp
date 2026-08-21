#include <iostream>

#include "core/setting/setting.hpp"

int main() {
  yabe::core::setting::Setting setting;
  std::cout << setting.generate_metafile_text();
}
