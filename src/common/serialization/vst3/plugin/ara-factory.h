// yabridge: a Wine plugin bridge
// Copyright (C) 2020-2026 Robbert van der Helm
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#ifdef WITH_ARA

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <ARAInterface.h>

#include "../../common.h"

// Callback invoked when the host calls ARAFactory::createDocumentControllerWithDocument.
// Receives the host instance, document properties, and a freshly allocated ara_dc_id.
using AraCreateDcFn =
    std::function<const ARA::ARADocumentControllerInstance*(
        const ARA::ARADocumentControllerHostInstance*,
        const ARA::ARADocumentProperties*,
        native_size_t ara_dc_id)>;

struct YaAraFactory {
    int32_t lowestSupportedApiGeneration = 0;
    int32_t highestSupportedApiGeneration = 0;
    uint64_t structSize = 0;
    int32_t supportedPlaybackTransformationFlags = 0;
    int32_t supportsStoringAudioFileChunks = 0;

    std::string factoryID;
    std::string plugInName;
    std::string manufacturerName;
    std::string informationURL;
    std::string version;
    std::string documentArchiveID;

    std::vector<std::string> compatibleDocumentArchiveIDs;
    std::vector<int32_t> analyzeableContentTypes;

    template <typename S>
    void serialize(S& s) {
        s.value4b(lowestSupportedApiGeneration);
        s.value4b(highestSupportedApiGeneration);
        s.value8b(structSize);
        s.value4b(supportedPlaybackTransformationFlags);
        s.value4b(supportsStoringAudioFileChunks);
        s.text1b(factoryID, 4096);
        s.text1b(plugInName, 4096);
        s.text1b(manufacturerName, 4096);
        s.text1b(informationURL, 4096);
        s.text1b(version, 4096);
        s.text1b(documentArchiveID, 4096);
        s.container(compatibleDocumentArchiveIDs, 256,
                    [](S& s, std::string& id) { s.text1b(id, 4096); });
        s.container4b(analyzeableContentTypes, 256);
    }

    // Build an ARAFactory C struct whose const char* pointers are backed by
    // this YaAraFactory's string members. Both this object and the returned
    // factory must remain at a stable address for the host's lifetime.
    //
    // If create_dc is provided, it is stored in one of the 8 trampoline slots
    // so the createDocumentControllerWithDocument trampoline can route to the
    // bridge. Slots are keyed by slot index and not by factory ID, so two
    // bridges exposing the same factory ID each get their own slot. Call
    // unregister_factory() on this exact object when the factory is no longer
    // needed. Copies of a registered object do not own its slot.
    //
    // When create_dc is null, or when all slots are in use, a stub returning
    // nullptr is installed instead.
    ARA::ARAFactory to_ara_factory(AraCreateDcFn create_dc = nullptr) const;

    // Free the trampoline slot claimed by to_ara_factory(), if any.
    void unregister_factory() const noexcept;

    YaAraFactory() = default;
    // Copies never take over the trampoline slot, so destroying or
    // unregistering a copy cannot disable the original's trampoline
    YaAraFactory(const YaAraFactory& other);
    YaAraFactory& operator=(const YaAraFactory& other);
    YaAraFactory(YaAraFactory&& other) noexcept;
    YaAraFactory& operator=(YaAraFactory&& other) noexcept;

   private:
    static void ARA_CALL stub_initialize(
        const ARA::ARAInterfaceConfiguration*) {}
    static void ARA_CALL stub_uninitialize() {}

    static const ARA::ARADocumentControllerInstance* ARA_CALL
    stub_create_document_controller(
        const ARA::ARADocumentControllerHostInstance*,
        const ARA::ARADocumentProperties*) {
        return nullptr;
    }

    void fill_factory_fields(ARA::ARAFactory& factory) const;

    mutable std::vector<const char*> compatible_archive_id_ptrs_;
    mutable std::vector<ARA::ARAContentType> content_type_ptrs_;
    // The trampoline slot claimed by to_ara_factory(), or -1
    mutable int slot_ = -1;
    mutable bool fill_called_ = false;
};

YaAraFactory from_ara_factory(const ARA::ARAFactory* factory);

struct YaAraPlugInExtensionInstance {
    bool has_playback_renderer = false;
    bool has_editor_renderer = false;
    bool has_editor_view = false;

    // Opaque renderer refs from the Wine plugin; sent back with every
    // renderer interface call so the Wine side can route to the right object.
    uint64_t playback_renderer_ref = 0;
    uint64_t editor_renderer_ref = 0;
    uint64_t editor_view_ref = 0;

    template <typename S>
    void serialize(S& s) {
        s.value1b(has_playback_renderer);
        s.value1b(has_editor_renderer);
        s.value1b(has_editor_view);
        s.value8b(playback_renderer_ref);
        s.value8b(editor_renderer_ref);
        s.value8b(editor_view_ref);
    }
};

#endif  // WITH_ARA
