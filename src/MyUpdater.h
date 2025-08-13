#ifndef MYUPDATER_H
#define MYUPDATER_H

#include "pch.h"
#include <AUI/Updater/AUpdater.h>
#include <AUI/Updater/AppropriatePortablePackagePredicate.h>
#include <AUI/Updater/GitHub.h>
#include <AUI/Updater/Semver.h>

static constexpr auto LOG_TAG = "MyUpdater";

static constexpr auto GH_OWNER = "Alex2772";
static constexpr auto GH_REPO = "neuro-organizer-app";

class MyUpdater final : public AUpdater {
public:
    ~MyUpdater() override = default;

protected:
    AFuture<void> checkForUpdatesImpl() override;
    AFuture<void> downloadUpdateImpl(const APath& unpackedUpdateDir) override;

private:
    AString mDownloadUrl;
    aui::updater::Semver mLatestVersion;
    AString mLatestTag;
    AString mAssetName;
};

#endif   // MYUPDATER_H
