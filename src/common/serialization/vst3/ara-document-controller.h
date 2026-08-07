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
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <ARAInterface.h>

#include "../../bitsery/ext/in-place-optional.h"
#include "../../bitsery/ext/in-place-variant.h"
#include "../common.h"
#include "base.h"
#include "plugin/ara-factory.h"

// Serializable ARA properties structs. All ARA*HostRef values that cross IPC
// boundaries are encoded as uint64_t handles. Plugin-side refs (ARA*Ref) are
// similarly encoded as uint64_t. All messages carry ara_dc_id to identify the
// document controller.

// ---------------------------------------------------------------------------
// Serializable property structs
// ---------------------------------------------------------------------------

struct YaAraColor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    template <typename S>
    void serialize(S& s) {
        s.value4b(r);
        s.value4b(g);
        s.value4b(b);
    }
};

struct YaAraDocumentProperties {
    std::string name;

    template <typename S>
    void serialize(S& s) {
        s.text1b(name, 4096);
    }
};

struct YaAraMusicalContextProperties {
    std::optional<std::string> name;
    int32_t order_index = 0;
    std::optional<YaAraColor> color;

    template <typename S>
    void serialize(S& s) {
        s.ext(name, bitsery::ext::InPlaceOptional{},
              [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.value4b(order_index);
        s.ext(color, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraColor& v) { s.object(v); });
    }
};

struct YaAraRegionSequenceProperties {
    std::optional<std::string> name;
    int32_t order_index = 0;
    uint64_t musical_context_ref = 0;
    std::optional<YaAraColor> color;

    template <typename S>
    void serialize(S& s) {
        s.ext(name, bitsery::ext::InPlaceOptional{},
              [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.value4b(order_index);
        s.value8b(musical_context_ref);
        s.ext(color, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraColor& v) { s.object(v); });
    }
};

struct YaAraSampleChannelArrangement {
    int32_t data_type = 0;
    std::vector<uint8_t> data;

    template <typename S>
    void serialize(S& s) {
        s.value4b(data_type);
        s.container1b(data, 65536);
    }
};

struct YaArAAudioSourceProperties {
    std::optional<std::string> name;
    std::string persistent_id;
    int64_t sample_count = 0;
    double sample_rate = 0.0;
    int32_t channel_count = 0;
    int32_t merits_64bit_samples = 0;
    std::optional<YaAraSampleChannelArrangement> channel_arrangement;

    template <typename S>
    void serialize(S& s) {
        s.ext(name, bitsery::ext::InPlaceOptional{},
              [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.text1b(persistent_id, 4096);
        s.value8b(sample_count);
        s.value8b(sample_rate);
        s.value4b(channel_count);
        s.value4b(merits_64bit_samples);
        s.ext(channel_arrangement, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraSampleChannelArrangement& v) { s.object(v); });
    }
};

struct YaArAAudioModificationProperties {
    std::optional<std::string> name;
    std::string persistent_id;

    template <typename S>
    void serialize(S& s) {
        s.ext(name, bitsery::ext::InPlaceOptional{},
              [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.text1b(persistent_id, 4096);
    }
};

struct YaAraPlaybackRegionProperties {
    int32_t transformation_flags = 0;
    double start_in_modification_time = 0.0;
    double duration_in_modification_time = 0.0;
    double start_in_playback_time = 0.0;
    double duration_in_playback_time = 0.0;
    // ARA1 deprecated field, still transmitted for compatibility
    uint64_t musical_context_ref = 0;
    uint64_t region_sequence_ref = 0;
    std::optional<std::string> name;
    std::optional<YaAraColor> color;

    template <typename S>
    void serialize(S& s) {
        s.value4b(transformation_flags);
        s.value8b(start_in_modification_time);
        s.value8b(duration_in_modification_time);
        s.value8b(start_in_playback_time);
        s.value8b(duration_in_playback_time);
        s.value8b(musical_context_ref);
        s.value8b(region_sequence_ref);
        s.ext(name, bitsery::ext::InPlaceOptional{},
              [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.ext(color, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraColor& v) { s.object(v); });
    }
};

struct YaAraContentTimeRange {
    double start = 0.0;
    double duration = 0.0;

    template <typename S>
    void serialize(S& s) {
        s.value8b(start);
        s.value8b(duration);
    }
};

struct YaAraRestoreObjectsFilter {
    int32_t document_data = 0;
    std::vector<std::string> audio_source_archive_ids;
    std::vector<std::string> audio_source_current_ids;
    std::vector<std::string> audio_modification_archive_ids;
    std::vector<std::string> audio_modification_current_ids;

    template <typename S>
    void serialize(S& s) {
        s.value4b(document_data);
        s.container(audio_source_archive_ids, 65536,
                    [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.container(audio_source_current_ids, 65536,
                    [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.container(audio_modification_archive_ids, 65536,
                    [](S& s, std::string& v) { s.text1b(v, 4096); });
        s.container(audio_modification_current_ids, 65536,
                    [](S& s, std::string& v) { s.text1b(v, 4096); });
    }
};

struct YaAraStoreObjectsFilter {
    int32_t document_data = 0;
    std::vector<uint64_t> audio_source_refs;
    std::vector<uint64_t> audio_modification_refs;

    template <typename S>
    void serialize(S& s) {
        s.value4b(document_data);
        s.container8b(audio_source_refs, 65536);
        s.container8b(audio_modification_refs, 65536);
    }
};

// ---------------------------------------------------------------------------
// Message structs for document controller IPC
// ---------------------------------------------------------------------------

namespace YaAra {

// -- Lifecycle ---------------------------------------------------------------

struct CreateDocumentController {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t instance_id;
    native_size_t ara_dc_id;
    std::string factory_id;
    YaAraDocumentProperties document_properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(instance_id);
        s.value8b(ara_dc_id);
        s.text1b(factory_id, 4096);
        s.object(document_properties);
    }
};

struct DestroyDocumentController {
    using Response = Ack;

    native_size_t ara_dc_id;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
    }
};

// -- Document editing --------------------------------------------------------

struct BeginEditing {
    using Response = Ack;

    native_size_t ara_dc_id;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
    }
};

struct EndEditing {
    using Response = Ack;

    native_size_t ara_dc_id;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
    }
};

struct NotifyModelUpdates {
    using Response = Ack;

    native_size_t ara_dc_id;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
    }
};

struct UpdateDocumentProperties {
    using Response = Ack;

    native_size_t ara_dc_id;
    YaAraDocumentProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.object(properties);
    }
};

// -- Musical context ---------------------------------------------------------

struct AddMusicalContext {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t host_ref;
    YaAraMusicalContextProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(host_ref);
        s.object(properties);
    }
};

struct UpdateMusicalContextProperties {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t musical_context_ref;
    YaAraMusicalContextProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(musical_context_ref);
        s.object(properties);
    }
};

struct UpdateMusicalContextContent {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t musical_context_ref;
    std::optional<YaAraContentTimeRange> range;
    int32_t flags;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(musical_context_ref);
        s.ext(range, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraContentTimeRange& v) { s.object(v); });
        s.value4b(flags);
    }
};

struct RemoveMusicalContext {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t musical_context_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(musical_context_ref);
    }
};

// -- Region sequences --------------------------------------------------------

struct AddRegionSequence {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t host_ref;
    YaAraRegionSequenceProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(host_ref);
        s.object(properties);
    }
};

struct UpdateRegionSequenceProperties {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t region_sequence_ref;
    YaAraRegionSequenceProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(region_sequence_ref);
        s.object(properties);
    }
};

struct RemoveRegionSequence {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t region_sequence_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(region_sequence_ref);
    }
};

// -- Audio source ------------------------------------------------------------

struct AddAudioSource {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t host_ref;
    YaArAAudioSourceProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(host_ref);
        s.object(properties);
    }
};

struct UpdateAudioSourceProperties {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;
    YaArAAudioSourceProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
        s.object(properties);
    }
};

struct UpdateAudioSourceContent {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;
    std::optional<YaAraContentTimeRange> range;
    int32_t flags;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
        s.ext(range, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraContentTimeRange& v) { s.object(v); });
        s.value4b(flags);
    }
};

struct EnableAudioSourceSamplesAccess {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;
    int32_t enable;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
        s.value4b(enable);
    }
};

struct DeactivateAndUnregisterAudioSource {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;
    int32_t deactivate;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
        s.value4b(deactivate);
    }
};

struct RemoveAudioSource {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
    }
};

// -- Audio modification ------------------------------------------------------

struct AddAudioModification {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;
    uint64_t host_ref;
    YaArAAudioModificationProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
        s.value8b(host_ref);
        s.object(properties);
    }
};

struct CloneAudioModification {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t audio_modification_ref;
    uint64_t host_ref;
    YaArAAudioModificationProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_modification_ref);
        s.value8b(host_ref);
        s.object(properties);
    }
};

struct UpdateAudioModificationProperties {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_modification_ref;
    YaArAAudioModificationProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_modification_ref);
        s.object(properties);
    }
};

struct DeactivateAndUnregisterAudioModification {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_modification_ref;
    int32_t deactivate;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_modification_ref);
        s.value4b(deactivate);
    }
};

struct RemoveAudioModification {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_modification_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_modification_ref);
    }
};

// -- Playback region ---------------------------------------------------------

struct AddPlaybackRegion {
    using Response = std::variant<uint64_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t audio_modification_ref;
    uint64_t host_ref;
    YaAraPlaybackRegionProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_modification_ref);
        s.value8b(host_ref);
        s.object(properties);
    }
};

struct UpdatePlaybackRegionProperties {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t playback_region_ref;
    YaAraPlaybackRegionProperties properties;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(playback_region_ref);
        s.object(properties);
    }
};

struct RemovePlaybackRegion {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t playback_region_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(playback_region_ref);
    }
};

// -- Content / analysis ------------------------------------------------------

struct RequestAudioSourceContentAnalysis {
    using Response = Ack;

    native_size_t ara_dc_id;
    uint64_t audio_source_ref;
    std::vector<int32_t> content_types;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(audio_source_ref);
        s.container4b(content_types, 256);
    }
};

struct GetPlaybackRegionHeadAndTailTime {
    struct Response {
        double head_time = 0.0;
        double tail_time = 0.0;

        template <typename S>
        void serialize(S& s) {
            s.value8b(head_time);
            s.value8b(tail_time);
        }
    };

    native_size_t ara_dc_id;
    uint64_t playback_region_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(playback_region_ref);
    }
};

// -- Archiving ---------------------------------------------------------------

struct StoreObjectsToArchive {
    using Response = std::variant<int32_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t archive_writer_host_ref;
    std::optional<YaAraStoreObjectsFilter> filter;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(archive_writer_host_ref);
        s.ext(filter, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraStoreObjectsFilter& v) { s.object(v); });
    }
};

struct RestoreObjectsFromArchive {
    using Response = std::variant<int32_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t archive_reader_host_ref;
    std::optional<YaAraRestoreObjectsFilter> filter;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(archive_reader_host_ref);
        s.ext(filter, bitsery::ext::InPlaceOptional{},
              [](S& s, YaAraRestoreObjectsFilter& v) { s.object(v); });
    }
};

struct StoreDocumentToArchive {
    using Response = std::variant<int32_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t archive_writer_host_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(archive_writer_host_ref);
    }
};

struct BeginRestoringDocumentFromArchive {
    using Response = std::variant<int32_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t archive_reader_host_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(archive_reader_host_ref);
    }
};

struct EndRestoringDocumentFromArchive {
    using Response = std::variant<int32_t, UniversalTResult>;

    native_size_t ara_dc_id;
    uint64_t archive_reader_host_ref;

    template <typename S>
    void serialize(S& s) {
        s.value8b(ara_dc_id);
        s.value8b(archive_reader_host_ref);
    }
};

}  // namespace YaAra

// ---------------------------------------------------------------------------
// Free serialization functions for response variants
// ---------------------------------------------------------------------------

// native_size_t is uint64_t, so this covers both AddMusicalContext-style
// responses and CreateDocumentController's response.
template <typename S>
void serialize(S& s, std::variant<uint64_t, UniversalTResult>& result) {
    s.ext(result,
          bitsery::ext::InPlaceVariant<bitsery::ext::OverloadValue<uint64_t,
                                                                    8>>{});
}

template <typename S>
void serialize(S& s, std::variant<int32_t, UniversalTResult>& result) {
    s.ext(result,
          bitsery::ext::InPlaceVariant<bitsery::ext::OverloadValue<int32_t,
                                                                    4>>{});
}

#endif  // WITH_ARA
