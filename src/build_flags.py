global_env = DefaultEnvironment()
global_env.Append(
    CPPDEFINES=[
        ("ARDUINOJSON_ENABLE_ARDUINO_STRING", 0),
        ("ARDUINOJSON_ENABLE_ARDUINO_STREAM", 0),
        ("ARDUINOJSON_ENABLE_ARDUINO_PRINT", 0),
        ("ARDUINOJSON_ENABLE_PROGMEM", 0),
        ("ATOMIC_FS_UPDATE", 1),
        ("PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED", 1),
    ]
)
global_env.Append(CXXFLAGS=["-std=c++17"])