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

// This header provides helpers for establishing the two named Unix domain
// socket connections used by the ARA SDK IPC layer inside yabridge.  The
// conventions mirror what yabridge already uses for all other VST3 sockets:
// the Wine host (plugin side) listens, and the Linux plugin host connects.
//
// Socket path conventions (relative to Vst3Sockets::base_dir_):
//   ara_ipc_main.sock   — main-thread ARA IPC channel
//   ara_ipc_other.sock  — other-threads ARA IPC channel

#include <string>

#include <ghc/filesystem.hpp>

/**
 * Return the path for the ARA IPC main-thread socket within the given
 * endpoint base directory.
 */
inline ghc::filesystem::path ara_ipc_main_socket_path(
    const ghc::filesystem::path& base_dir) {
    return base_dir / "ara_ipc_main.sock";
}

/**
 * Return the path for the ARA IPC other-threads socket within the given
 * endpoint base directory.
 */
inline ghc::filesystem::path ara_ipc_other_socket_path(
    const ghc::filesystem::path& base_dir) {
    return base_dir / "ara_ipc_other.sock";
}
