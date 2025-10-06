#include "tiny_gltf_loader.h"

#ifdef __ANDROID__
  #include <android/asset_manager.h>
#endif
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#ifdef __ANDROID__
void setAssetmanager(AAssetManager* assetManager) {
  tinygltf::asset_manager = assetManager;
}
#endif
