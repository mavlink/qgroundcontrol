QT.core.VERSION = 6.10.3
QT.core.name = QtCore
QT.core.module = Qt6Core
QT.core.libs = $$QT_MODULE_LIB_BASE
QT.core.ldflags = 
QT.core.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtCore
QT.core.frameworks = 
QT.core.bins = $$QT_MODULE_BIN_BASE
QT.core.depends =  
QT.core.uses = libatomic
QT.core.module_config = v2
QT.core.CONFIG = moc resources
QT.core.DEFINES = QT_CORE_LIB
QT.core.enabled_features = clock-monotonic cxx11_future cxx17_filesystem glib inotify std-atomic64 mimetype regularexpression sharedmemory shortcut systemsemaphore xmlstream xmlstreamreader xmlstreamwriter textdate datestring process processenvironment temporaryfile library settings filesystemwatcher filesystemiterator itemmodel proxymodel sortfilterproxymodel identityproxymodel transposeproxymodel concatenatetablesproxymodel stringlistmodel translation easingcurve animation gestures jalalicalendar islamiccivilcalendar timezone commandlineparser cborstreamreader cborstreamwriter permissions threadsafe-cloexec version_tagging shared pkg-config separate_debug_info rpath reduce_relocations signaling_nan zstd thread future concurrent dbus opensslv30 test_gui shared intelcet glibc_fortify_source stack_protector stack_clash_protection libstdcpp_assertions relro_now_linker shared separate_debug_info rpath reduce_exports reduce_relocations openssl
QT.core.disabled_features = jemalloc cpp-winrt timezone_tzdb static cross_compile debug_and_release appstore-compliant simulator_and_device force_asserts framework c++20 c++2a c++2b c++2c wasm-simd128 wasm-exceptions wasm-jspi openssl-linked opensslv11
QT_CONFIG += clock-monotonic cxx11_future cxx17_filesystem glib inotify std-atomic64 mimetype regularexpression sharedmemory shortcut systemsemaphore xmlstream xmlstreamreader xmlstreamwriter textdate datestring process processenvironment temporaryfile library settings filesystemwatcher filesystemiterator itemmodel proxymodel sortfilterproxymodel identityproxymodel transposeproxymodel concatenatetablesproxymodel stringlistmodel translation easingcurve animation gestures jalalicalendar islamiccivilcalendar timezone commandlineparser cborstreamreader cborstreamwriter permissions threadsafe-cloexec version_tagging shared pkg-config separate_debug_info rpath reduce_relocations signaling_nan zstd thread future concurrent dbus opensslv30 test_gui shared intelcet glibc_fortify_source stack_protector stack_clash_protection libstdcpp_assertions relro_now_linker shared separate_debug_info rpath reduce_exports reduce_relocations openssl
QT_MODULES += core

