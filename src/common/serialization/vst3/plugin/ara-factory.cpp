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

#ifdef WITH_ARA

#include "ara-factory.h"

YaAraFactory from_ara_factory(const ARA::ARAFactory* factory) {
    YaAraFactory result{};

    result.structSize = static_cast<uint64_t>(factory->structSize);
    result.lowestSupportedApiGeneration =
        static_cast<int32_t>(factory->lowestSupportedApiGeneration);
    result.highestSupportedApiGeneration =
        static_cast<int32_t>(factory->highestSupportedApiGeneration);
    result.factoryID = factory->factoryID ? factory->factoryID : "";
    result.plugInName = factory->plugInName ? factory->plugInName : "";
    result.manufacturerName =
        factory->manufacturerName ? factory->manufacturerName : "";
    result.informationURL =
        factory->informationURL ? factory->informationURL : "";
    result.version = factory->version ? factory->version : "";
    result.documentArchiveID =
        factory->documentArchiveID ? factory->documentArchiveID : "";

    if (factory->compatibleDocumentArchiveIDs) {
        result.compatibleDocumentArchiveIDs.reserve(
            factory->compatibleDocumentArchiveIDsCount);
        for (ARA::ARASize i = 0;
             i < factory->compatibleDocumentArchiveIDsCount; ++i) {
            result.compatibleDocumentArchiveIDs.push_back(
                factory->compatibleDocumentArchiveIDs[i]
                    ? factory->compatibleDocumentArchiveIDs[i]
                    : "");
        }
    }

    if (factory->analyzeableContentTypes) {
        result.analyzeableContentTypes.reserve(
            factory->analyzeableContentTypesCount);
        for (ARA::ARASize i = 0;
             i < factory->analyzeableContentTypesCount; ++i) {
            result.analyzeableContentTypes.push_back(
                static_cast<int32_t>(factory->analyzeableContentTypes[i]));
        }
    }

    result.supportedPlaybackTransformationFlags =
        static_cast<int32_t>(factory->supportedPlaybackTransformationFlags);
    result.supportsStoringAudioFileChunks =
        static_cast<int32_t>(factory->supportsStoringAudioFileChunks);

    return result;
}

#endif  // WITH_ARA
