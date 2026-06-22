/// @file main.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <iostream>

#ifndef PRINTING_MESSAGE

#define PRINTING_MESSAGE "No cmake presets config"

#endif

int main() {
  std::cout << "CMake preset name: " PRINTING_MESSAGE << std::endl;

  return 0;
}