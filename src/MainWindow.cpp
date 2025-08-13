#include "pch.h"
#include "stringUtils.h"
#include "MainWindow.h"

class TitleTextArea;
using namespace declarative;
using namespace ass;

struct Note {
    AProperty<AString> title;
    AProperty<AString> content;
    AProperty<AString> textFilePath;
    AProperty<AString> imageFilePath;
    AProperty<AString> timestamp;
    AProperty<AString> base64;
};

AJSON_FIELDS(
    Note,
    AJSON_FIELDS_ENTRY(title) AJSON_FIELDS_ENTRY(content) AJSON_FIELDS_ENTRY(textFilePath)
        AJSON_FIELDS_ENTRY(imageFilePath) AJSON_FIELDS_ENTRY(timestamp) AJSON_FIELDS_ENTRY(base64))

static constexpr auto NOTES_SORT_BY_TITLE = ranges::actions::sort(std::less {}, [](const _<Note>& n) {
    return n->title->lowercase();
});

class TitleTextArea : public ATextArea {
public:
    using ATextArea::ATextArea;
    void onCharEntered(const char16_t c) override {
        if (c == '\r') {
            AWindow::current()->focusNextView();
            return;
        }
        ATextArea::onCharEntered(c);
    }
};

MainWindow::MainWindow(_<MyUpdater> updater) :
    AWindow("Notes"), mUpdater(std::move(updater)) {
    allowDragNDrop();
    setExtraStylesheet(AStylesheet {
      { c(".plain_bg"), BackgroundSolid { AColor::WHITE } },
      { t<AWindow>(), Padding { 0 } },
    });

    load();
    connect(mNotes.changed, this, &MainWindow::markDirty);

    AObject::connect(mNotes.changed, [this] { recomputeFiltered(); });
    AObject::connect(mSearchQuery.changed, [this] { recomputeFiltered(); });
    recomputeFiltered();

    setContents(Vertical {
      ASplitter::Horizontal()
          .withItems({

            // 🟦 Left panel
            Vertical {
              Centered {
                Horizontal {
                  Button { Icon { ":img/remove.svg" } AUI_WITH_STYLE { MinSize { 40_dp, 40_dp } },
                           Label { "Remove Note" } AUI_WITH_STYLE { FontSize { 20_pt } } }
                          .connect(&AView::clicked, this, &MainWindow::removeCurrentNote) &
                      mCurrentNote.readProjected([](const _<Note>& n) { return n != nullptr; }) > &AView::setEnabled,
                  Button { Icon { ":img/new.svg" } AUI_WITH_STYLE { MinSize { 40_dp, 40_dp } },
                           Label { "Add Note" } AUI_WITH_STYLE { FontSize { 20_pt } } }
                      .connect(&AView::clicked, this, &MainWindow::newNote),
                },
              },

              Horizontal {
                Icon { ":img/search.svg" } AUI_WITH_STYLE { MinSize { 40_dp, 40_dp } },
                Label { "Search" } AUI_WITH_STYLE { FontSize { 20_pt } },
                _new<ATextArea>("Search notes...") AUI_LET {
                        AObject::biConnect(mSearchQuery, it->text());
                        it->setCustomStyle({ FontSize { 20_pt }, Expanding { 1, 0 } });
                    },
              },

                Horizontal {} AUI_WITH_STYLE {
                    MinSize { 0_dp, 1_dp }, BackgroundSolid { AColor::BLACK }, Margin { 4_dp, 0_dp }
                },

                CustomLayout {} & mUpdater->status.readProjected([&updater = mUpdater](const std::any& status) -> _<AView> {
                    if (std::any_cast<AUpdater::StatusIdle>(&status)) {
                        return _new<AButton>("Check for updates").connect(&AView::clicked, AUI_SLOT(updater)::checkForUpdates);
                    }
                    if (std::any_cast<AUpdater::StatusCheckingForUpdates>(&status)) {
                        return Label { "Checking for updates..." };
                    }
                    if (auto downloading = std::any_cast<AUpdater::StatusDownloading>(&status)) {
                        return Vertical {
                            Label { "Downloading..." },
                            _new<AProgressBar>() & downloading->progress,
                        };
                    }
                    if (std::any_cast<AUpdater::StatusWaitingForApplyAndRestart>(&status)) {
                        return _new<AButton>("Apply update and restart")
                            .connect(&AView::clicked, AUI_SLOT(updater)::applyUpdateAndRestart);
                    }
                    return nullptr;
                }),


              Horizontal {} AUI_WITH_STYLE {
                MinSize { 0_dp, 1_dp }, BackgroundSolid { AColor::BLACK }, Margin { 4_dp, 0_dp } },

              AScrollArea::Builder()
                  .withContents(
                      AUI_DECLARATIVE_FOR(note, *mFilteredNotes, AVerticalLayout) {
                      observeChangesForDirty(note);
                      return notePreview(note) AUI_LET {
                          connect(it->clicked, [this, note] { mCurrentNote = note; });
                          it& mCurrentNote > [note](AView& view, const _<Note>& currentNote) {
                              ALOG_DEBUG("CHECK") << "currentNote == note " << currentNote << " == " << note;
                              view.setAssName(".plain_bg", currentNote == note);
                          };
                      };
                  })
                  .build(),
            } AUI_WITH_STYLE { MinSize { 200_dp } },

            // 🟥 Right panel
            Vertical::Expanding {
              CustomLayout::Expanding {} &
              mCurrentNote.readProjected([this](const _<Note>& note) -> _<AView> { return noteEditor(note); }) }
                << ".plain_bg" AUI_WITH_STYLE { MinSize { 200_dp } },

          })
          .build() AUI_WITH_STYLE { Expanding() },
    });

    if (mNotes->empty()) {
        newNote();
    } else if (*mCurrentNote == nullptr && !mNotes->empty()) {
        mCurrentNote = (*mNotes)[0];
    }
}

MainWindow::~MainWindow() {
    if (mDirty) {
        save();
    }
}

void MainWindow::load() {
    try {
        if (!"notes.json"_path.isRegularFileExists()) {
            return;
        }
        aui::from_json(AJson::fromStream(AFileInputStream("notes.json")), mNotes);
    } catch (const AException& e) {
        ALogger::info("MainWindow") << "Can't load notes: " << e;
    }
}

void MainWindow::save() {
    AFileOutputStream("notes.json") << aui::to_json(*mNotes);
    mDirty = false;
}

void MainWindow::newNote() {
    auto note = aui::ptr::manage_shared(new Note { .title = "Untitled" });
    mNotes.writeScope()->push_back(note);
    mCurrentNote = note;
    mSearchQuery = "";
}

void MainWindow::removeCurrentNote() {
    if (mCurrentNote == nullptr) {
        return;
    }

    const auto noteToRemove = *mCurrentNote;

    if (AMessageBox::show(
            this, "Remove Note",
            "Do you really want to remove this note?\n\n{}\n\nThis operation is irreversible!"_format(
                noteToRemove->title),
            AMessageBox::Icon::WARNING, AMessageBox::Button::OK_CANCEL) != AMessageBox::ResultButton::OK) {
        return;
    }

    auto& notes = *mNotes.writeScope();
    auto it = ranges::find(notes, noteToRemove);
    if (it != notes.end()) {
        it = notes.erase(it);
        if (!notes.empty()) {
            auto removed_timestamp_folder_by_imageFilePath = APath { (*mCurrentNote)->imageFilePath }.parent();
            if (removed_timestamp_folder_by_imageFilePath.exists())
                removed_timestamp_folder_by_imageFilePath.removeFileRecursive();
            auto removed_timestamp_folder_by_textFilePath = APath { (*mCurrentNote)->textFilePath }.parent();
            if (removed_timestamp_folder_by_textFilePath.exists())
                removed_timestamp_folder_by_textFilePath.removeFileRecursive();
            mCurrentNote = (it != notes.end()) ? *it : notes.back();
        } else {
            (*mCurrentNote)->title = "Untitled";
            (*mCurrentNote)->content = "";
            (*mCurrentNote)->base64 = "";
            (*mCurrentNote)->imageFilePath = "";
            (*mCurrentNote)->textFilePath = "";
            (*mCurrentNote)->timestamp = "";
            mCurrentNote = nullptr;
            auto reports = APath("reports");
            reports.removeFileRecursive();
            auto notes_json = APath("notes.json");
            notes_json.removeFile();
        }

        markDirty();
        recomputeFiltered();
    }
}

void MainWindow::markDirty() {
    mDirty = true;
    if (mCurrentNote == nullptr)
        return;
    auto note = *mCurrentNote;
    if (note->timestamp->empty()) {
        auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto tm = std::localtime(&time_now);
        std::ostringstream timestamp;
        timestamp << std::put_time(tm, "%Y-%m-%d_%H-%M-%S");
        note->timestamp = timestamp.str();
        auto folder = APath("reports") / APath(note->timestamp);
        folder.makeDirs();
        note->textFilePath = folder / "info.txt";
    }
    auto textFilePath = APath(note->textFilePath);
    textFilePath.parent().makeDirs();
    AFileOutputStream fos(textFilePath);
    fos << note->title->toStdString() << "\n\n" << note->content->toStdString();
}

void MainWindow::observeChangesForDirty(const _<Note>& note) {
    aui::reflect::for_each_field_value(
        *note,
        aui::lambda_overloaded {
          [&](auto& field) {},
          [&](APropertyReadable auto& field) { AObject::connect(field.changed, this, &MainWindow::markDirty); },
        });
}

_<AView> MainWindow::noteEditor(const _<Note>& note) {
    if (note == nullptr) {
        return Centered { Label { "No note selected" } };
    }

    return AScrollArea::Builder().withContents(Vertical {
        Label {} AUI_LET {
            it->setCustomStyle({ FontSize { 10_pt }, ATextOverflow::ELLIPSIS });
            auto updateFolder = [it, note] {
                AString img = *note->imageFilePath;
                AString txt = *note->textFilePath;
                AString shown;
                if (!img.empty()) {
                    APath ip(img);
                    if (ip.exists()) {
                        shown = ip.parent();
                    }
                }
                if (shown.empty() && !txt.empty()) {
                    APath tp(txt);
                    shown = tp.parent();
                }
                if (shown.empty())
                    shown = "(folder has not been created)";
                it->setText(shown);
            };
            updateFolder();
            AObject::connect(note->imageFilePath.changed, [updateFolder] { updateFolder(); });
            AObject::connect(note->textFilePath.changed, [updateFolder] { updateFolder(); });
        },

        Horizontal {} AUI_WITH_STYLE {
            MinSize { 0_dp, 1_dp },
            BackgroundSolid { AColor::BLACK },
            Margin { 4_dp, 0_dp }
        },

        _new<ATextArea>("Text") AUI_WITH_STYLE {
            FontSize { 14_pt },
            Expanding()
        } && note->content,

        _new<ADrawableView>() AUI_LET {
            it->setCustomStyle({
                FixedSize { {}, 400_dp },
                BackgroundImage{{}, {}, {}, Sizing::CONTAIN }
            });

            auto updatePreview = [it, note] {
                const AString& img = *note->imageFilePath;
                if (!img.empty()) {
                    it->setDrawable(IDrawable::fromUrl(img));
                } else {
                    it->setDrawable(nullptr);
                }
            };
            updatePreview();
            AObject::connect(note->imageFilePath.changed, [updatePreview] { updatePreview(); });
        },

        _new<ADragNDropView>() AUI_WITH_STYLE {
            BackgroundSolid { AColor::TRANSPARENT_WHITE },
            MaxSize { 0_pt, 0_pt },
            Margin { 0 }
        },
    });
}

_<AView> MainWindow::notePreview(const _<Note>& note) {
    return Vertical {
        _new<TitleTextArea>(*note->title) AUI_LET {
            it->setCustomStyle({
                FontSize { 14_pt },
                Border { 0 },
                Padding { 0 },
                Margin { 0 },
                Expanding(),
            });
            AObject::biConnect(note->title, it->text());
        },
    } AUI_WITH_STYLE {
        Padding { 4_dp, 8_dp },
        BorderRadius { 8_dp },
        Margin { 4_dp, 8_dp },
    };
}

void MainWindow::recomputeFiltered() {
    const auto& notes = *mNotes;
    const std::string q = utf8_fold(mSearchQuery->toStdString());

    const auto searchFilter = ranges::views::filter([&](const _<Note>& note) {
        const std::string titleF = utf8_fold(note->title->toStdString());
        const std::string contentF = utf8_fold(note->content->toStdString());
        return q.empty() || titleF.find(q) != std::string::npos || contentF.find(q) != std::string::npos;
    });

    mFilteredNotes = notes | searchFilter | ranges::to<AVector<_<Note>>>() | NOTES_SORT_BY_TITLE;
}

void MainWindow::onDragDrop(const ADragNDrop::DropEvent& event) {
    AWindow::onDragDrop(event);

    for (const auto& [k, v] : event.data.data()) {
        ALogger::info("Drop") << "[" << k << "] = " << AString::fromUtf8(v);
    }

    [&]() -> _<AView> {
        if (auto u = event.data.urls()) {
            auto url = u->first();
            auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            auto tm = std::localtime(&time_now);
            std::ostringstream timestamp;
            timestamp << std::put_time(tm, "%Y-%m-%d_%H-%M-%S");
            auto p = APath("reports") / APath(timestamp.str());
            if ((*mCurrentNote)->timestamp->empty()) {
                p.makeDirs();
                (*mCurrentNote)->timestamp = timestamp.str();
            } else {
                p = APath("reports") / APath((*mCurrentNote)->timestamp);
                auto checking_picture = APath { (*mCurrentNote)->imageFilePath };
                if (checking_picture.exists()) {
                    checking_picture.removeFile();
                }
            }
            AFileOutputStream fos(p / "info.txt");
            fos << (*mCurrentNote)->title->toStdString() << "\n\n" << (*mCurrentNote)->content->toStdString();
            AString srcStr = url.full();
            APath srcPath;
            if (srcStr.startsWith("file://")) {
                AString s = srcStr;
                if (s.startsWith("file:///"))
                    s = s.substr(8);
                else
                    s = s.substr(7);
                srcPath = APath { s };
            } else {
                srcPath = APath { srcStr };
            }
            APath dstPath = p / srcPath.filename();
            try {
                if (auto icon = AImageDrawable::fromUrl(srcPath)) {
                    try {
                        auto buf = AByteBuffer::fromStream(AFileInputStream(srcPath));
                        AFileOutputStream out(dstPath);
                        out.write(buf.data(), buf.size());
                        ALogger::info("Report") << "Image copied to: " << dstPath;
                    } catch (const AException& e) {
                        ALogger::warn("Report") << "Failed to copy image from " << srcPath << " to " << dstPath << ": " << e;
                    }
                    (*mCurrentNote)->imageFilePath = dstPath;
                    (*mCurrentNote)->base64 = AByteBuffer::fromStream(AFileInputStream(dstPath)).toBase64String();
                    markDirty();
                }
            }
            catch (const AException& e) {
                ALogger::warn("Report") << "Failed to load source image from " << srcPath << ": " << e;
                return Centered {};
            }
            try {
                if (auto icon = AImageDrawable::fromUrl(url)) {
                    return
                    Centered {
                        _new<ADrawableView>(icon) AUI_WITH_STYLE {
                            FixedSize { {}, 400_dp },
                            BackgroundImage{{}, {}, {}, Sizing::CONTAIN }
                        }
                    };
                }
            }
            catch (const AException& e) {
                ALogger::warn("Report") << "Failed to render image from " << url.full() << ": " << e;
                return Centered {};
            }
        }
        return nullptr;
    }();
}

bool MainWindow::onDragEnter(const ADragNDrop::EnterEvent& event) { return mCurrentNote != nullptr; }
