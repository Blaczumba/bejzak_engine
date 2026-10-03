#include "android_platform.h"

#include <android_native_app_glue.h>
#include <array>
#include <jni.h>
#include <openxr/openxr_platform.h>
#include <spdlog/spdlog.h>

namespace {

void AppHandleCmd(struct android_app* app, int32_t cmd) {
  auto* appState = reinterpret_cast<xrw::AndroidAppState*>(app->userData);
  switch (cmd) {
    case APP_CMD_START:
      {
        spdlog::info("APP_CMD_START onStart()");
        break;
      }
    case APP_CMD_RESUME:
      {
        spdlog::info("APP_CMD_RESUME onResume()");
        appState->resumed = true;
        break;
      }
    case APP_CMD_PAUSE:
      {
        spdlog::info("APP_CMD_PAUSE onPause()");
        appState->resumed = false;
        break;
      }
    case APP_CMD_STOP:
      {
        spdlog::info("APP_CMD_STOP onStop()");
        break;
      }
    case APP_CMD_DESTROY:
      {
        spdlog::info("APP_CMD_DESTROY onDestroy()");
        break;
      }
    case APP_CMD_INIT_WINDOW:
      {
        spdlog::info("APP_CMD_INIT_WINDOW surfaceCreated()");
        break;
      }
    case APP_CMD_TERM_WINDOW:
      {
        spdlog::info("APP_CMD_TERM_WINDOW surfaceDestroyed()");
        break;
      }
  }
}

}  // namespace

namespace xrw {

AndroidPlatform::AndroidPlatform(struct android_app* app) : _app(app) {
  _app->activity->vm->AttachCurrentThread(&_env, nullptr);
  _app->userData = &_appState;
  _app->onAppCmd = AppHandleCmd;

  PFN_xrInitializeLoaderKHR initialize_loader = nullptr;
  if (XR_SUCCEEDED(xrGetInstanceProcAddr(
          XR_NULL_HANDLE, "xrInitializeLoaderKHR",
          reinterpret_cast<PFN_xrVoidFunction*>(&initialize_loader)))) [[likely]] {
    XrLoaderInitInfoAndroidKHR loader_init_info_android = {
      .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
      .applicationVM = _app->activity->vm,
      .applicationContext = _app->activity->clazz};
    initialize_loader(
        reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loader_init_info_android));
  }

  _instance_create_info_android = XrInstanceCreateInfoAndroidKHR{
    .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
    .applicationVM = _app->activity->vm,
    .applicationActivity = _app->activity->clazz};
}

std::span<const char* const> AndroidPlatform::getInstanceExtensions() const {
  static constexpr std::array platformExtensions = {XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
  return platformExtensions;
}

const XrBaseInStructure* AndroidPlatform::getInstanceCreateExtension() const {
  return reinterpret_cast<const XrBaseInStructure*>(&_instance_create_info_android);
}

bool AndroidPlatform::shouldClose() const {
  return _app->destroyRequested != 0;
}

void AndroidPlatform::pollPlatformEvents(bool applicationRunning) {
  int events, timeout;
  struct android_poll_source* source;
  while (true) {
    timeout = (!_appState.resumed && !applicationRunning && !shouldClose()) ? -1 : 0;
    if (ALooper_pollOnce(timeout, nullptr, &events, (void**)&source) < 0) {
      break;
    }

    if (source != nullptr) {
      source->process(_app, source);
    }
  }
}

}  // namespace xrw
