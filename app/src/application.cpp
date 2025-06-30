

#include <ranges>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <spdlog/spdlog.h>

#include <device.hpp>
#include <queue.hpp>

#include "application.hpp"
#include "device_manager.hpp"
#include "ui/device_discovery_window.hpp"
#include "vk_utils.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {

inline void check_vk_result(VkResult err) {
  if (err != VK_SUCCESS)
    throw std::runtime_error("VkError");
}

#ifdef APP_USE_VULKAN_DEBUG_UTILS
inline constexpr std::array validation_layers{"VK_LAYER_KHRONOS_validation"};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *) {
  const auto level =
      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
          ? spdlog::level::err
      : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
          ? spdlog::level::warn
      : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
          ? spdlog::level::info
          : spdlog::level::debug;
  spdlog::log(level, "[Vulkan] {} (id: {}): {}",
              data->pMessageIdName ? data->pMessageIdName : "-",
              data->messageIdNumber, data->pMessage);

  return VK_FALSE;
}
#endif

inline constexpr std::array device_extensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    VK_KHR_MAINTENANCE2_EXTENSION_NAME};

inline constexpr std::array validation_features_enable{
    vk::ValidationFeatureEnableEXT::eDebugPrintf};

inline constexpr auto application_info =
    vk::ApplicationInfo{}
        .setPApplicationName("VK Application")
        .setApplicationVersion(VK_MAKE_API_VERSION(0, 1, 0, 0))
        .setPEngineName("VK Engine")
        .setEngineVersion(VK_MAKE_API_VERSION(0, 1, 0, 0))
        .setApiVersion(VK_API_VERSION_1_4);

} // namespace

application::application() {
  init_window();
  init_vk();
  create_swapchain();
  init_imgui();
  create_swapchain_views();
  create_frames();

  device_open_subscription_ = event_bus_.subscribe<device_open_requested>(
      [this](const device_open_requested &event) {
        spdlog::info("Device open requested for");
      });
}

application::~application() {
  event_bus_.unsubscribe(device_open_subscription_);
}

void application::glfw_error_callback(int error, const char *description) {
  spdlog::error("GLFW Error {}: {}", error, description);
}

void application::init_window() {
  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit())
    throw std::runtime_error("glfwInit failed");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(1280, 720, "VK Application", nullptr, nullptr);
  if (!window_)
    throw std::runtime_error("glfwCreateWindow failed");
}

void application::init_vk() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  uint32_t     glfw_ext_count = 0;
  const char **glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
  std::vector<const char *> instance_extensions(glfw_ext,
                                                glfw_ext + glfw_ext_count);

#ifdef APP_USE_VULKAN_DEBUG_UTILS
  instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  instance_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
#endif
  instance_extensions.push_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  instance_ = std::make_shared<engine::instance>(engine::instance(
      std::span{validation_features_enable},
      std::span<vk::ValidationFeatureDisableEXT>{}, instance_extensions,
      std::span{validation_layers}, {}, application_info));

  VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_->handle());

#ifdef APP_USE_VULKAN_DEBUG_UTILS
  debug_utils_messenger_ =
      instance_->handle().createDebugUtilsMessengerEXTUnique(
          vk::DebugUtilsMessengerCreateInfoEXT()
              .setMessageSeverity(
                  vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                  vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                  vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                  vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
              .setMessageType(
                  vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                  vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                  vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
              .setPfnUserCallback(debug_utils_messenger_callback));
#endif
  surface_ = std::make_shared<engine::surface>(window_, instance_);

  gpus_ = engine::instance::enumerate_gpus(instance_);

  auto gpu_expected = vk_utils::pick_discrete_gpu(gpus_);
  if (!gpu_expected)
    throw std::runtime_error(gpu_expected.error());
  selected_gpu_ = *gpu_expected;

  auto fam_exp = vk_utils::pick_queue_families(*selected_gpu_);
  if (!fam_exp)
    throw std::runtime_error(fam_exp.error());

  const auto                             families = *fam_exp;
  constexpr float                        prio = 1.0f;
  std::vector<vk::DeviceQueueCreateInfo> qcis;
  std::set<uint32_t> unique_families{families.graphics, families.compute,
                                     families.transfer};
  for (uint32_t idx : unique_families) {
    qcis.push_back(
        vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(idx).setQueuePriorities(
            prio));
  }

  engine::feature_chain feats;
  feats.get<vk::PhysicalDeviceVulkan13Features>()
      .setDynamicRendering(true)
      .setSynchronization2(true);

  feats.get<vk::DeviceCreateInfo>()
      .setQueueCreateInfos(qcis)
      .setPEnabledExtensionNames(device_extensions);

  engine::device_bundle device_bundle =
      engine::device::create(selected_gpu_, feats, device_extensions);

  device_ = std::move(device_bundle.dev);
  graphics_queue_ = std::move(device_bundle.queues[0]);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(device_->handle());

  std::array<vk::DescriptorPoolSize, 1> pool_sizes = {{vk::DescriptorPoolSize(
      vk::DescriptorType::eCombinedImageSampler,
      IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE)}};

  descriptor_pool_ = device_->handle().createDescriptorPoolUnique(
      vk::DescriptorPoolCreateInfo()
          .setPoolSizes(pool_sizes)
          .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
          .setMaxSets(1));
}

void application::create_swapchain() {
  vk::Format requested_formats[] = {
      vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm,
      vk::Format::eB8G8R8Unorm, vk::Format::eR8G8B8Unorm};

  vk::ColorSpaceKHR requested_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
  vk::SurfaceFormatKHR surface_format = select_surface_format(
      std::vector<vk::Format>(std::begin(requested_formats),
                              std::end(requested_formats)),
      requested_color_space);

  const auto caps =
      selected_gpu_->handle().getSurfaceCapabilitiesKHR(surface_->handle());

  uint32_t image_count = caps.minImageCount + 1;
  if (caps.maxImageCount && image_count > caps.maxImageCount)
    image_count = caps.maxImageCount;

  vk::Extent2D extent;
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
    extent = caps.currentExtent;
  else {
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    extent.width =
        std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width,
                   caps.maxImageExtent.width);
    extent.height =
        std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height,
                   caps.maxImageExtent.height);
  }
  swapchain_extent_ = extent;
  const auto present_mode = choose_present_mode(
      selected_gpu_->handle().getSurfacePresentModesKHR(surface_->handle()));

  swapchain_ = std::make_unique<engine::swapchain>(
      device_, surface_,
      vk::SwapchainCreateInfoKHR()
          .setSurface(surface_->handle())
          .setMinImageCount(image_count)
          .setImageFormat(surface_format.format)
          .setImageColorSpace(surface_format.colorSpace)
          .setImageExtent(extent)
          .setImageArrayLayers(1)
          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
          .setImageSharingMode(vk::SharingMode::eExclusive)
          .setPreTransform(caps.currentTransform)
          .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
          .setPresentMode(present_mode)
          .setClipped(VK_TRUE),
      surface_format);
}

void application::init_imgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGuiIO &io = ImGui::GetIO();

  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui_ImplGlfw_InitForVulkan(window_, true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.PipelineRenderingCreateInfo =
      vk::PipelineRenderingCreateInfo().setColorAttachmentFormats(
          swapchain_->surface_format().format);
  init_info.UseDynamicRendering = true;
  init_info.Instance = instance_->handle();
  init_info.PhysicalDevice = selected_gpu_->handle();
  init_info.Device = device_->handle();
  init_info.QueueFamily = graphics_queue_->queue_family_index();
  init_info.Queue = graphics_queue_->handle();
  init_info.DescriptorPool = *descriptor_pool_;
  init_info.Subpass = 0;
  init_info.MinImageCount = swapchain_->min_image_count();
  init_info.ImageCount = swapchain_->images().size();
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = check_vk_result;

  if (!ImGui_ImplVulkan_Init(&init_info))
    throw std::runtime_error("Failed to init ImGui with Vulkan");
}

void application::create_swapchain_views() {
  swapchain_views_.clear();
  swapchain_views_.reserve(swapchain_->images().size());
  for (const auto &img : swapchain_->images()) {
    swapchain_views_.push_back(device_->handle().createImageViewUnique(
        vk::ImageViewCreateInfo{}
            .setImage(img.handle)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(swapchain_->surface_format().format)
            .setComponents(vk::ComponentMapping{})
            .setSubresourceRange(
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
  }
}

void application::create_frames() {
  const uint32_t image_count =
      static_cast<uint32_t>(swapchain_->images().size());
  frames_.resize(image_count);

  for (auto &f : frames_) {
    f.command_pool = device_->handle().createCommandPoolUnique(
        {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
         graphics_queue_->queue_family_index()});
    f.cmd = device_->handle()
                .allocateCommandBuffers(
                    {*f.command_pool, vk::CommandBufferLevel::ePrimary, 1})
                .front();

    f.image_available = device_->handle().createSemaphoreUnique({});
    f.render_finished = device_->handle().createSemaphoreUnique({});
    f.in_flight = device_->handle().createFenceUnique(
        {vk::FenceCreateFlagBits::eSignaled});
  }
}

void application::recreate_swapchain() {
  device_->handle().waitIdle();

  frames_.clear();
  swapchain_views_.clear();
  swapchain_.reset();

  create_swapchain();
  create_swapchain_views();
  create_frames();

  ImGui_ImplVulkan_SetMinImageCount(swapchain_->min_image_count());
}

void application::record(frame &f, uint32_t img_idx) {
  f.cmd.reset({});
  f.cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  // ── PRESENT_SRC -> COLOR_ATTACHMENT_OPTIMAL ─────────────
  vk::ImageMemoryBarrier2 pre{};
  pre.setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
      .setSrcAccessMask(vk::AccessFlagBits2::eNone)
      .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
      .setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setImage(swapchain_->images()[img_idx].handle)
      .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  f.cmd.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(pre));

  // ── Dynamic Rendering begin ─────────────────────────────
  const vk::RenderingAttachmentInfo color =
      vk::RenderingAttachmentInfo{}
          .setImageView(*swapchain_views_[img_idx])
          .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eStore)
          .setClearValue(
              vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.f}));

  f.cmd.beginRendering(vk::RenderingInfo{}
                           .setRenderArea({{0, 0}, swapchain_extent_})
                           .setLayerCount(1)
                           .setColorAttachments(color));

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), f.cmd);
  f.cmd.endRendering();

  // ── COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC ────────────
  vk::ImageMemoryBarrier2 post{};
  post.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
      .setDstAccessMask(vk::AccessFlagBits2::eNone)
      .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
      .setImage(swapchain_->images()[img_idx].handle)
      .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  f.cmd.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(post));

  f.cmd.end();
}

void application::run() {
  bool rebuild_swapchain = false;

  device_manager          device_manager(event_bus_);
  device_discovery_window device_discovery_window(device_manager);

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->Pos);
      ImGui::SetNextWindowSize(viewport->Size);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
      window_flags |= ImGuiWindowFlags_NoTitleBar |
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoMove;
      window_flags |=
          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
      window_flags |= ImGuiWindowFlags_NoBackground;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("DockSpace", nullptr, window_flags);
      ImGui::PopStyleVar(3);

      // DockSpace
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                       ImGuiDockNodeFlags_None);

      device_discovery_window.render();

      ImGui::End();
    }

    ImGui::Render();

    if (rebuild_swapchain) {
      recreate_swapchain();
      rebuild_swapchain = false;
    }

    frame &f = frames_[cur_frame_];
    device_->handle().waitForFences(*f.in_flight, VK_TRUE, UINT64_MAX);

    auto acq = swapchain_->acquire_image(*f.image_available,
                                         std::chrono::milliseconds(500));
    if (!acq) {
      rebuild_swapchain = true;
      continue;
    }

    const uint32_t img_idx = acq->image_index;
    device_->handle().resetFences(*f.in_flight);

    record(f, img_idx);

    vk::PipelineStageFlags wait =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit(*f.image_available, wait, f.cmd, *f.render_finished);
    graphics_queue_->handle().submit(submit, *f.in_flight);

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
    }

    auto present_res = swapchain_->present(img_idx, *f.render_finished,
                                           graphics_queue_->handle());
    if (!present_res) {
      rebuild_swapchain = true;
      continue;
    }

    cur_frame_ = (cur_frame_ + 1) % frames_.size();
  }

  device_->handle().waitIdle();
}