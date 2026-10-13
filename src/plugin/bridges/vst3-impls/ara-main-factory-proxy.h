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

#include <ARAVST3.h>

#include "../../../common/serialization/vst3/plugin/ara-factory.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

class Vst3PluginBridge;

class YaMainFactoryImpl : public ARA::IMainFactory {
   public:
    YaMainFactoryImpl(Vst3PluginBridge& bridge,
                      YaAraFactory&& factory) noexcept;

    virtual ~YaMainFactoryImpl() noexcept;

    YaMainFactoryImpl(const YaMainFactoryImpl&) = delete;
    YaMainFactoryImpl& operator=(const YaMainFactoryImpl&) = delete;
    YaMainFactoryImpl(YaMainFactoryImpl&&) = delete;
    YaMainFactoryImpl& operator=(YaMainFactoryImpl&&) = delete;

    DECLARE_FUNKNOWN_METHODS

    const ARA::ARAFactory* PLUGIN_API getFactory() override;

    /**
     * The factory data and the `ARAFactory` handed to the host. ARA requires
     * the `ARAFactory` pointer returned by `getFactory()` to stay valid for as
     * long as the module is loaded, and hosts like Studio One release the
     * `IMainFactory` right after calling `getFactory()` and only later call
     * `createDocumentControllerWithDocument()` through that pointer. So this
     * state is owned by a process-wide registry instead of by this object,
     * and it is shared between all `IMainFactory` instances for the same
     * bridge and factory ID.
     */
    struct PersistentFactory {
        YaAraFactory factory;
        ARA::ARAFactory ara_factory{};
    };

    /**
     * Get the persistent factory for this bridge and `factory.factoryID`,
     * creating it on first use. Plugin instances use this for
     * `IPlugInEntryPoint::getFactory()` as well, so there is only a single
     * registered `createDocumentControllerWithDocument()` trampoline per
     * factory. Registering one per object made later registrations replace
     * the trampoline slot of the `ARAFactory` the host already had.
     */
    static PersistentFactory& get_persistent_factory(Vst3PluginBridge& bridge,
                                                     YaAraFactory&& factory);

   private:
    Vst3PluginBridge& bridge_;
    PersistentFactory* state_;
};

#pragma GCC diagnostic pop

#endif  // WITH_ARA
