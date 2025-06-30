add_executable(app 
    src/main.cpp
    src/application.cpp
    src/device_manager.cpp
    src/ui/device_discovery_window.cpp
    src/ui/feature_list_view.cpp
    src/ui/gev_device_control_window.cpp
    )

FetchContent_Declare(
    sl_device
    GIT_REPOSITORY https://dev.azure.com/SpectrumLogic/SLDevice/_git/c_api
    GIT_TAG        master
)
FetchContent_MakeAvailable(sl_device)

find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)

target_link_libraries(app
    PRIVATE
        vulkan_engine
        spdlog::spdlog
        imgui::imgui
        nfd::nfd
        sl_device
)

target_include_directories(app PRIVATE src/)

target_compile_definitions(app
    PRIVATE
        "VKENGINE_SHADER_DIR=\"${SHADER_OUTPUT_DIR}\""
        $<$<CONFIG:Debug>:APP_USE_VULKAN_DEBUG_UTILS>
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)