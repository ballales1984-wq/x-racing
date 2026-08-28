#include "track_blueprint.h"
#include <iostream>

int main() {
  std::cout << "X-Racing Track Blueprint Editor\n";
  std::cout << "================================\n\n";
  std::cout << "Tools:\n";
  std::cout << "  V - Select tool\n";
  std::cout << "  A - Add vertex\n";
  std::cout << "  S - Add straight\n";
  std::cout << "  L - Add left curve\n";
  std::cout << "  R - Add right curve\n";
  std::cout << "  P - Add pit box\n";
  std::cout << "  F - Set start/finish\n";
  std::cout << "  B - Add barrier\n";
  std::cout << "  E - Eraser\n";
  std::cout << "\nActions:\n";
  std::cout << "  C - Close loop\n";
  std::cout << "  G - Toggle grid\n";
  std::cout << "  N - Clear track\n";
  std::cout << "  Z - Undo\n";
  std::cout << "  Del - Delete selected\n";
  std::cout << "  Scroll - Zoom\n";
  std::cout << "\nFile Menu:\n";
  std::cout << "  Export SVG / Export JSON\n\n";

  p0::track_blueprint::BlueprintEditor editor("Nuovo Autodromo");
  if (!editor.initialize()) {
    std::cerr << "Failed to initialize blueprint editor\n";
    return 1;
  }

  int result = editor.run();
  return result;
}
