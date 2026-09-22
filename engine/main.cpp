#include <memory>
#include <print>

#include "bejzak_engine/common/file/standard_file_loader.h"
#include "bejzak_engine/common/util/engine_exception.h"
#include "bejzak_engine/common/window/window_glfw.h"
#include "bejzak_engine/vulkan/graphics_context/presentation.h"
#include "bejzak_engine/vulkan/wrapper/util/check.h"

int main() {
  auto fileLoader = std::make_unique<StandardFileLoader>();
  std::unique_ptr<common::Presentation> application;
  try {
    application = vlkn::Presentation::create(
        std::make_unique<WindowGlfw>("BejzakEngine", 1920, 1080, false), *fileLoader);
    application->run();
  } catch (const EngineException& engineException) {
    std::println("Engine exception occured with message: {}. \n", engineException.what());
  } catch (const VkException& vkException) {
    std::println("Vulkan exception occured with message: {} and VkResult code: {}",
                 vkException.what(), static_cast<std::int32_t>(vkException.getResult()));
  }

  return EXIT_SUCCESS;
}
