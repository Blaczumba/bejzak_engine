#pragma once

#include <command_buffer/command_buffer.h>
#include <debug_messenger/debug_messenger.h>
#include <instance/instance.h>
#include <logical_device/logical_device.h>
#include <physical_device/physical_device.h>
#include <swapchain/swapchain.h>
#include <window/window.h>
#include <pipeline/shader/shader_program.h>

class ApplicationBase {
protected:
    std::unique_ptr<Instance> _instance;
#ifdef VALIDATION_LAYERS_ENABLED
    std::unique_ptr<DebugMessenger> _debugMessenger;
#endif // VALIDATION_LAYERS_ENABLED
    std::unique_ptr<Window> _window;
    std::unique_ptr<Surface> _surface;
    std::unique_ptr<PhysicalDevice> _physicalDevice;
    std::unique_ptr<LogicalDevice> _logicalDevice;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<CommandPool> _singleTimeCommandPool;
    std::unique_ptr<ShaderProgramManager> _programManager;

public:
    ApplicationBase();
    virtual ~ApplicationBase() = default;
    virtual void run() = 0;
};
