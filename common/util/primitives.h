#pragma once

#include <glm/glm.hpp>

struct UniformBufferLight {
  alignas(16) glm::mat4 projView;
  alignas(16) glm::vec3 pos;
};

struct UniformBufferCamera {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 pos;
  alignas(16) glm::vec3 viewDir;
};
