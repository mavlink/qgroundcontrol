add_qgc_tests(AudioOutputTest LABELS Unit Utilities)

# PiperTtsEngine is only built on Linux
if(LINUX)
    add_qgc_tests(PiperTtsEngineTest LABELS Unit Utilities)
endif()
