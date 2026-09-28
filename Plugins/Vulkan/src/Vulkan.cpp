#include "Vulkan.hpp"

namespace Vk {

void Device::configure(NodeBuild node) {
    auto backend = node.require<Vulkan>();
    physicalDevice_ = backend->physicalDevice(info_.id);
    createDevice();
    createCommandPool();
}

void Device::createDevice() {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            queueFamily_ = i;
            break;
        }
    }
    if (queueFamily_ == UINT32_MAX)
        throw Lattice::Exception<Device>("physical device has no usable queue");

    constexpr float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.queueFamilyIndex = queueFamily_;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;

    VkDeviceCreateInfo desc{};
    desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    desc.queueCreateInfoCount = 1;
    desc.pQueueCreateInfos = &queue;

    if (vkCreateDevice(physicalDevice_, &desc, nullptr, &device_) != VK_SUCCESS)
        throw Lattice::Exception<Device>("failed to create logical device");

    vkGetDeviceQueue(device_, queueFamily_, 0, &queue_);
}

void Device::createCommandPool() {
    VkCommandPoolCreateInfo desc{};
    desc.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    desc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    desc.queueFamilyIndex = queueFamily_;
    if (vkCreateCommandPool(device_, &desc, nullptr, &commandPool_) != VK_SUCCESS)
        throw Lattice::Exception<Device>("failed to create command pool");
}

void Vulkan::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Lattice";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Lattice";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo desc{};
    desc.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    desc.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&desc, nullptr, &instance_) != VK_SUCCESS)
        throw Lattice::Exception<Vulkan>("failed to create Vulkan instance");
}


std::vector<VkPhysicalDevice> Vulkan::enumeratePhysicalDevices() {
    uint32_t count = 0;

    if (vkEnumeratePhysicalDevices(instance_, &count, nullptr) != VK_SUCCESS)
        throw Lattice::Exception<Vulkan>("failed to enumerate physical devices");

    if (count == 0)
        throw Lattice::Exception<Vulkan>("no Vulkan physical devices found");

    std::vector<VkPhysicalDevice> devices(count);

    if (vkEnumeratePhysicalDevices(instance_, &count, devices.data()) != VK_SUCCESS)
        throw Lattice::Exception<Vulkan>("failed to enumerate physical devices");

    return devices;
}


VkPhysicalDevice Vulkan::selectPhysicalDevice(std::span<VkPhysicalDevice> devices) {
    if (devices.empty())
        throw Lattice::Exception<Vulkan>("no physical devices available");

    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(device, &props);

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }

    return devices.front();
}


GPU::DeviceDesc Vulkan::describeDevice(VkPhysicalDevice physical) const {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical, &props);

    uint32_t id = 0;

    for (; id < physicalDevices_.size(); ++id) {
        if (physicalDevices_[id] == physical)
            break;
    }

    if (id == physicalDevices_.size())
        throw Lattice::Exception<Vulkan>("physical device is not registered");

    GPU::DeviceDesc desc{};
    desc.id = id;
    desc.name = props.deviceName;
    desc.type = deviceType(props.deviceType);

    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceMemoryProperties(physical, &memory);

    for (uint32_t i = 0; i < memory.memoryHeapCount; ++i) {
        if (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            desc.memory += memory.memoryHeaps[i].size;
    }

    return desc;
}

VkDevice Vulkan::createDevice(VkPhysicalDevice physical) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, families.data());

    uint32_t queueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamily = i;
            break;
        }
    }

    if (queueFamily == UINT32_MAX)
        throw Lattice::Exception<Vulkan>("physical device has no compute queue");

    constexpr float priority = 1.0f;

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo desc{};
    desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    desc.queueCreateInfoCount = 1;
    desc.pQueueCreateInfos = &queueInfo;

    VkDevice device = VK_NULL_HANDLE;

    if (vkCreateDevice(physical, &desc, nullptr, &device) != VK_SUCCESS)
        throw Lattice::Exception<Vulkan>("failed to create logical device");

    return device;
}

}
