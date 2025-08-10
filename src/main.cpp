#include "pch.h"
#include "MainWindow.h"

using namespace declarative;
using namespace ass;

AUI_ENTRY {
    auto w = _new<MainWindow>();
    w->show();
    return 0;
}
