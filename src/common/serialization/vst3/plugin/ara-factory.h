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

#include <cstdint>
#include <string>
#include <vector>

#include <ARAInterface.h>

struct YaAraFactory {
    // Stored as plain integer types to avoid bitsery issues with ARA_32_BIT_ENUM
    // structs and platform-dependent size_t (ARASize).
    int32_t lowestSupportedApiGeneration;
    int32_t highestSupportedApiGeneration;
    uint64_t structSize;
    int32_t supportedPlaybackTransformationFlags;
    int32_t supportsStoringAudioFileChunks;

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

    ARA::ARAFactory to_ara_factory() const noexcept {
        compatible_archive_id_ptrs_.clear();
        compatible_archive_id_ptrs_.reserve(compatibleDocumentArchiveIDs.size());
        for (const auto& id : compatibleDocumentArchiveIDs) {
            compatible_archive_id_ptrs_.push_back(id.c_str());
        }

        ARA::ARAFactory factory{};
        factory.structSize = static_cast<ARA::ARASize>(structSize);
        factory.lowestSupportedApiGeneration =
            static_cast<ARA::ARAAPIGeneration>(lowestSupportedApiGeneration);
        factory.highestSupportedApiGeneration =
            static_cast<ARA::ARAAPIGeneration>(highestSupportedApiGeneration);
        factory.factoryID = factoryID.c_str();
        factory.initializeARAWithConfiguration = stub_initialize;
        factory.uninitializeARA = stub_uninitialize;
        factory.plugInName = plugInName.c_str();
        factory.manufacturerName = manufacturerName.c_str();
        factory.informationURL = informationURL.c_str();
        factory.version = version.c_str();
        factory.createDocumentControllerWithDocument =
            stub_create_document_controller;
        factory.documentArchiveID = documentArchiveID.c_str();
        factory.compatibleDocumentArchiveIDsCount =
            static_cast<ARA::ARASize>(compatible_archive_id_ptrs_.size());
        factory.compatibleDocumentArchiveIDs =
            compatible_archive_id_ptrs_.empty()
                ? nullptr
                : compatible_archive_id_ptrs_.data();
        factory.analyzeableContentTypesCount =
            static_cast<ARA::ARASize>(analyzeableContentTypes.size());

        content_type_ptrs_.resize(analyzeableContentTypes.size());
        for (size_t i = 0; i < analyzeableContentTypes.size(); ++i) {
            content_type_ptrs_[i] =
                static_cast<ARA::ARAContentType>(analyzeableContentTypes[i]);
        }
        factory.analyzeableContentTypes =
            content_type_ptrs_.empty() ? nullptr : content_type_ptrs_.data();

        factory.supportedPlaybackTransformationFlags =
            static_cast<ARA::ARAPlaybackTransformationFlags>(
                supportedPlaybackTransformationFlags);
        factory.supportsStoringAudioFileChunks =
            static_cast<ARA::ARABool>(supportsStoringAudioFileChunks);
        return factory;
    }

   private:
    // Stub callbacks — replaced by real implementations once document
    // controller support (task 7) is wired up.
    static void ARA_CALL stub_initialize(
        const ARA::ARAInterfaceConfiguration* /*config*/) {}
    static void ARA_CALL stub_uninitialize() {}
    static const ARA::ARADocumentControllerInstance* ARA_CALL
    stub_create_document_controller(
        const ARA::ARADocumentControllerHostInstance* /*hostInstance*/,
        const ARA::ARADocumentProperties* /*properties*/) {
        return nullptr;
    }

    mutable std::vector<const char*> compatible_archive_id_ptrs_;
    mutable std::vector<ARA::ARAContentType> content_type_ptrs_;
};

YaAraFactory from_ara_factory(const ARA::ARAFactory* factory);

struct YaAraPlugInExtensionInstance {
    bool has_playback_renderer = false;
    bool has_editor_renderer = false;
    bool has_editor_view = false;

    template <typename S>
    void serialize(S& s) {
        s.value1b(has_playback_renderer);
        s.value1b(has_editor_renderer);
        s.value1b(has_editor_view);
    }
};

#endif  // WITH_ARA
