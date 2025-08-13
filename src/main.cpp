#include "pch.h"
#include "MainWindow.h"

using namespace declarative;
using namespace ass;

AUI_ENTRY {
    auto updater = _new<MyUpdater>();
    updater->handleStartup(args);

    _new<MainWindow>(std::move(updater))->show();
    return 0;
}
