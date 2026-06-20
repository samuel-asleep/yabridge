# Meson does not allow mixing native and non native dependencies from
# subprojects. The only workaround is to only define only the necessary
# variables there, and to then assemble the dependencies here ourselves.

vst3 = subproject('vst3', version : '3.7.7')

# The ARA SDK is a git submodule at subprojects/ara-sdk/.  We build the
# ARA IPC static libraries here using the existing relative include path.
#
# Note: the include_directories() call will emit a sandbox violation warning
# because it points outside this subdirectory, but it functions correctly
# and is suppressed from yabridge's own warnings by is_system:true.
# This matches the pre-existing ara_sdk_inc usage in this file.
ara_sdk_inc = include_directories('../../../subprojects/ara-sdk', is_system : true)

# Also add yabridge's own ARA IPC transport headers (SocketChannel/SocketEncoder)
# to the include path.  These are in src/common/ara-ipc/ and tracked by the
# parent repo so they are always available.
ara_ipc_own_inc = include_directories('../../common/ara-ipc', is_system : true)

ara_sdk_compile_args = ['-DARA_ENABLE_IPC=1']

# Source files for the Linux (host/plugin native) side: Connection + ProxyPlugIn
# All files are in src/common/ara-ipc/ (yabridge's own source tree) so there
# is no Meson subproject sandbox issue.
ara_ipc_plugin_sources = files(
  '../../common/ara-ipc/ARAIPCConnection.cpp',
  '../../common/ara-ipc/ARAIPCProxyPlugIn.cpp',
  '../../common/ara-ipc/ARAHostDispatch.cpp',
  '../../common/ara-ipc/ARAPlugInDispatch.cpp',
)

# Source files for the Wine host side: Connection + ProxyHost
ara_ipc_host_sources = files(
  '../../common/ara-ipc/ARAIPCConnection.cpp',
  '../../common/ara-ipc/ARAIPCProxyHost.cpp',
  '../../common/ara-ipc/ARAHostDispatch.cpp',
  '../../common/ara-ipc/ARAPlugInDispatch.cpp',
)

# Native (Linux) ARA IPC static library — linked into libyabridge-vst3.so
ara_ipc_native = static_library(
  'ara_ipc_native',
  ara_ipc_plugin_sources,
  native : true,
  include_directories : [ara_sdk_inc, ara_ipc_own_inc],
  cpp_args : ara_sdk_compile_args + ['-Wno-deprecated-declarations'],
  override_options : ['warning_level=0'],
)

ara_ipc_native_dep = declare_dependency(
  link_with : ara_ipc_native,
  include_directories : [ara_sdk_inc, ara_ipc_own_inc],
  compile_args : ara_sdk_compile_args + ['-Wno-deprecated-declarations'],
)

# Cross (Wine) ARA IPC static library — linked into yabridge-host.exe
if is_64bit_system
  ara_ipc_wine_64bit = static_library(
    'ara_ipc_wine_64bit',
    ara_ipc_host_sources,
    native : false,
    include_directories : [ara_sdk_inc, ara_ipc_own_inc],
    cpp_args : ara_sdk_compile_args + wine_64bit_compiler_options + ['-U__i386__', '-Wno-deprecated-declarations'],
    override_options : ['warning_level=0'],
  )
  ara_ipc_wine_64bit_dep = declare_dependency(
    link_with : ara_ipc_wine_64bit,
    include_directories : [ara_sdk_inc, ara_ipc_own_inc],
    compile_args : ara_sdk_compile_args + ['-U__i386__', '-Wno-deprecated-declarations'],
  )
endif

if with_bitbridge
  # The ARA SDK IPC encoding layer uses vtable offsets that assume 64-bit
  # pointer layout.  32-bit Wine builds disable the ARA IPC layer entirely
  # via YABRIDGE_DISABLE_ARA_IPC; the include dirs are still exposed so
  # the headers compile (they're guarded by the macro).
  ara_ipc_wine_32bit_dep = declare_dependency(
    include_directories : [ara_sdk_inc, ara_ipc_own_inc],
    compile_args : ara_sdk_compile_args + ['-DYABRIDGE_DISABLE_ARA_IPC', '-Wno-deprecated-declarations'],
  )
endif

# Also include our normal compiler options so symbols are hidden by default
vst3_compiler_options = compiler_options + vst3.get_variable('compiler_options') + ['-DARA_ENABLE_IPC=1']
vst3_include_dir = vst3.get_variable('include_dir')

# We need to do some minor hacking to get the SDK to compile with winegcc. Most
# notably some attributes are named differently, and the SDK uses 'Windows.h'
# instead of 'windows.h' like how the file is actually called.
# message(vst3_include_dir)
vst3_sdk_base_dir = vst3.get_variable('sdk_base_dir')
patch_result = run_command('../../../tools/patch-vst3-sdk.sh', vst3_sdk_base_dir, check : true)
message(patch_result.stdout())

# We'll create a dependency for the plugin SDK for our native VST3 plugin
vst3_base_native = static_library(
  'base_native',
  vst3.get_variable('base_sources'),
  native : true,
  cpp_args : vst3_compiler_options + ['-Wno-cpp'],
  include_directories : vst3_include_dir,
  override_options : ['warning_level=0'],
)
vst3_pluginterfaces_native = static_library(
  'pluginterfaces_native',
  vst3.get_variable('pluginterfaces_sources'),
  native : true,
  cpp_args : vst3_compiler_options,
  include_directories : vst3_include_dir,
  override_options : ['warning_level=0'],
)
vst3_sdk_native = static_library(
  'sdk_native',
  vst3.get_variable('sdk_common_sources') + vst3.get_variable('sdk_sources'),
  native : true,
  link_with : [vst3_base_native, vst3_pluginterfaces_native],
  cpp_args : vst3_compiler_options + ['-Wno-multichar'],
  include_directories : vst3_include_dir,
  override_options : ['warning_level=0'],
)

# We need a separate host SDK dependency for our Wine plugin host applications
if is_64bit_system
  vst3_base_wine_64bit = static_library(
    'vst3_base_wine_64bit',
    vst3.get_variable('base_sources'),
    native : false,
    cpp_args : vst3_compiler_options + wine_64bit_compiler_options + [ '-Wno-cpp'],
    include_directories : vst3_include_dir,
    override_options : ['warning_level=0'],
  )
  vst3_pluginterfaces_wine_64bit = static_library(
    'vst3_pluginterfaces_wine_64bit',
    vst3.get_variable('pluginterfaces_sources'),
    native : false,
    cpp_args : vst3_compiler_options + wine_64bit_compiler_options,
    include_directories : vst3_include_dir,
    override_options : ['warning_level=0'],
  )
  vst3_sdk_hosting_wine_64bit = static_library(
    'vst3_sdk_hosting_wine_64bit',
    vst3.get_variable('sdk_common_sources') + vst3.get_variable('sdk_hosting_sources'),
    native : false,
    link_with : [vst3_base_wine_64bit, vst3_pluginterfaces_wine_64bit],
    cpp_args : vst3_compiler_options + wine_64bit_compiler_options + ['-Wno-multichar'],
    include_directories : vst3_include_dir,
    override_options : ['warning_level=0'],
  )
endif

# And another time for the 32-bit version
if with_bitbridge
  vst3_base_wine_32bit = static_library(
    'vst3_base_wine_32bit',
    vst3.get_variable('base_sources'),
    native : false,
    cpp_args : vst3_compiler_options + wine_32bit_compiler_options + ['-Wno-cpp'],
    include_directories : vst3_include_dir,
    override_options : ['warning_level=0'],
  )
  vst3_pluginterfaces_wine_32bit = static_library(
    'vst3_pluginterfaces_wine_32bit',
    vst3.get_variable('pluginterfaces_sources'),
    native : false,
    cpp_args : vst3_compiler_options + wine_32bit_compiler_options,
    include_directories : vst3_include_dir,
    override_options : ['warning_level=0'],
  )
  vst3_sdk_hosting_wine_32bit = static_library(
    'vst3_sdk_hosting_wine_32bit',
    vst3.get_variable('sdk_common_sources') + vst3.get_variable('sdk_hosting_sources'),
    native : false,
    link_with : [vst3_base_wine_32bit, vst3_pluginterfaces_wine_32bit],
    cpp_args : vst3_compiler_options + wine_32bit_compiler_options + ['-Wno-multichar'],
    include_directories : vst3_include_dir,
    override_options : ['warning_level=0'],
  )
endif

#
# VST3 dependencies
#

vst3_sdk_native_dep = declare_dependency(
  link_with : vst3_sdk_native,
  include_directories : [vst3_include_dir, ara_sdk_inc],
  compile_args : vst3_compiler_options,
  dependencies : [ara_ipc_native_dep],
)

if is_64bit_system
  vst3_sdk_hosting_wine_64bit_dep = declare_dependency(
    link_with : vst3_sdk_hosting_wine_64bit,
    include_directories : [vst3_include_dir, ara_sdk_inc],
    # This does mean that we now have a lot of defines in our code, but the
    # alternative would be patching every location in the SDK where they include
    # `windows.h`
    compile_args : vst3_compiler_options + wine_64bit_compiler_options,
    dependencies : [ara_ipc_wine_64bit_dep],
  )
endif

if with_bitbridge
  vst3_sdk_hosting_wine_32bit_dep = declare_dependency(
    link_with : vst3_sdk_hosting_wine_32bit,
    include_directories : [vst3_include_dir, ara_sdk_inc],
    # This does mean that we now have a lot of defines in our code, but the
    # alternative would be patching every location in the SDK where they include
    # `windows.h`
    compile_args : vst3_compiler_options + wine_32bit_compiler_options,
    dependencies : [ara_ipc_wine_32bit_dep],
  )
endif
