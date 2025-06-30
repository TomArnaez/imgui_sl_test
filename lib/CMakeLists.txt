add_library(vulkan_engine STATIC
    src/device.cpp
    src/instance.cpp
    src/queue.cpp
    src/surface.cpp)

# FetchContent_Declare(
#     slang
#     GIT_REPOSITORY https://github.com/shader-slang/slang.git
#     GIT_TAG        master
# )

# set(SLANG_LIB_TYPE STATIC)

# set(SLANG_ENABLE_DXIL OFF)
# set(SLANG_ENABLE_GFX OFF)
# set(SLANG_ENABLE_SLANGD OFF)
# set(SLANG_ENABLE_SLANGC OFF)
# set(SLANG_ENABLE_TESTS OFF)
# set(SLANG_ENABLE_RELEASE_DEBUG_INFO OFF)
# set(SLANG_ENABLE_SLANG_RHI OFF)
# set(SLANG_ENABLE_CUDA OFF)
# set(SLANG_ENABLE_OPTIX OFF)
# set(SLANG_ENABLE_NVAPI OFF)
# set(SLANG_ENABLE_AFTERMATH OFF)
# set(SLANG_ENABLE_EXAMPLES OFF)
# set(SLANG_ENABLE_XLIB OFF)

# FetchContent_MakeAvailable(slang)

find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(vulkan_engine
    PUBLIC
        Vulkan::Vulkan
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator
)

target_compile_definitions(vulkan_engine
    PUBLIC
        GLFW_INCLUDE_VULKAN)

target_include_directories(vulkan_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(vulkan_engine 
    PUBLIC
        cxx_std_23
)