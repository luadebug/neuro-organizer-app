#include "pch.h"
#include "MainWindow.h"

using namespace declarative;
using namespace ass;

AUI_ENTRY {
    auto updater = _new<MyUpdater>();
    updater->handleStartup(args);

    const auto w = _new<MainWindow>(std::move(updater));
    w->show();
    return 0;
}
