#pragma once

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <utility>

namespace engine {

template <bool IsDeviceAddressable> struct device_address_holder;

template <> struct device_address_holder<true> {
  vk::DeviceAddress addr_{};

  void              set(vk::DeviceAddress a) noexcept { addr_ = a; }
  vk::DeviceAddress get() const noexcept {
    assert(addr_ != 0 && "Device address was requested but is not valid!");
    return addr_;
  }
};

template <> struct device_address_holder<false> {};

template <VkBufferUsageFlags       UsageFlags,
          VmaAllocationCreateFlags AllocationFlags>
class buffer {
public:
  static constexpr vk::BufferUsageFlags usage =
      static_cast<vk::BufferUsageFlags>(UsageFlags);
  static constexpr bool is_device_addressable =
      (UsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0;

  buffer(size_t size, vk::Device device, VmaAllocator allocator)
      : size_(size), allocator_(allocator) {

    vk::BufferCreateInfo create_info{};
    create_info.size = size_;
    create_info.usage = usage;
    create_info.sharingMode = sharing;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.flags = AllocationFlags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateBuffer(
            allocator_,
            reinterpret_cast<const VkBufferCreateInfo *>(&create_info),
            &alloc_info, &handle_, &allocation_,
            &allocation_info_) != VK_SUCCESS) {
      throw std::runtime_error{"vmaCreateBuffer failed"};
    }

    handle_ = raw;

    if constexpr (is_device_addressable) {
      address_.set(device.getBufferAddress(
          vk::BufferDeviceAddressInfo{}.setBuffer(handle_)));
    }
  }

  [[nodiscard]] vk::Buffer handle() const noexcept { return handle_; }
  [[nodiscard]] size_t     size() const noexcept { return size_; }

  [[nodiscard]] vk::DeviceAddress device_address() const noexcept
    requires(is_device_addressable) {
    return address_.get();
  }

private:
  size_t            size_;
  vk::Buffer        handle_;
  VmaAllocation     allocation_;
  VmaAllocationInfo allocation_info_;

  [[no_unique_address]]
  device_address_holder<is_device_addressable> address_;
};

} // namespace engine