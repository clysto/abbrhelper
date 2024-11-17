#include "mainwindow.hpp"
#include "utils.hpp"

int main(int argc, char* argv[]) {
  load_all_journals();
  auto app = QApplication{argc, argv};
  auto window = MainWindow{};
  window.show();
  return app.exec();
}
