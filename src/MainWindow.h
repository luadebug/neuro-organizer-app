#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "pch.h"
#include <AUI/Platform/AWindow.h>
#include <AUI/Common/AProperty.h>
#include <AUI/Common/AString.h>
#include <AUI/Common/AVector.h>
#include <AUI/Platform/ADesktop.h>
#include <AUI/View/ADragNDropView.h>
#include <AUI/View/AText.h>

struct Note;

class MainWindow : public AWindow {
public:
    MainWindow();
    ~MainWindow() override;

    void load();
    void save();
    void newNote();
    void removeCurrentNote();
    void markDirty();
    void recomputeFiltered();

private:
    void observeChangesForDirty(const _<Note>& note);
    _<AView> noteEditor(const _<Note>& note);
    _<AView> notePreview(const _<Note>& note);
    void onDragDrop(const ADragNDrop::DropEvent& event) override;
    bool onDragEnter(const ADragNDrop::EnterEvent& event);

    AProperty<AVector<_<Note>>> mNotes;
    AProperty<_<Note>> mCurrentNote;
    AProperty<bool> mDirty = false;
    AProperty<AString> mSearchQuery;
    AProperty<AVector<_<Note>>> mFilteredNotes;
};

#endif   // MAINWINDOW_H
