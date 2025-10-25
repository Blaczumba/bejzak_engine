#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct Extent {
  uint32_t width;
  uint32_t height;
};

struct VertexP {
  glm::vec3 pos;

  static constexpr size_t num_attributes = 1;
};

struct VertexPT {
  glm::vec3 pos;
  glm::vec2 texCoord;

  static constexpr size_t num_attributes = 2;
};

struct VertexPTN {
  glm::vec3 pos;
  glm::vec2 texCoord;
  glm::vec3 normal;

  static constexpr size_t num_attributes = 3;
};

struct VertexPTNT {
  glm::vec3 pos;
  glm::vec2 texCoord;
  glm::vec3 normal;
  glm::vec3 tangent;

  static constexpr size_t num_attributes = 4;
};

struct VertexPTNTB {
  glm::vec3 pos;
  glm::vec2 texCoord;
  glm::vec3 normal;
  glm::vec3 tangent;
  glm::vec3 bitangent;

  static constexpr size_t num_attributes = 5;
};

template <typename VertexType>
struct VertexTraits;

template <>
struct VertexTraits<VertexP> {
  static constexpr bool hasPosition = true;
  static constexpr bool hasTexCoord = false;
  static constexpr bool hasNormal = false;
  static constexpr bool hasTangent = false;
  static constexpr bool hasBitangent = false;
};

template <>
struct VertexTraits<VertexPT> {
  static constexpr bool hasPosition = true;
  static constexpr bool hasTexCoord = true;
  static constexpr bool hasNormal = false;
  static constexpr bool hasTangent = false;
  static constexpr bool hasBitangent = false;
};

template <>
struct VertexTraits<VertexPTN> {
  static constexpr bool hasPosition = true;
  static constexpr bool hasTexCoord = true;
  static constexpr bool hasNormal = true;
  static constexpr bool hasTangent = false;
  static constexpr bool hasBitangent = false;
};

template <>
struct VertexTraits<VertexPTNT> {
  static constexpr bool hasPosition = true;
  static constexpr bool hasTexCoord = true;
  static constexpr bool hasNormal = true;
  static constexpr bool hasTangent = true;
  static constexpr bool hasBitangent = false;
};

template <>
struct VertexTraits<VertexPTNTB> {
  static constexpr bool hasPosition = true;
  static constexpr bool hasTexCoord = true;
  static constexpr bool hasNormal = true;
  static constexpr bool hasTangent = true;
  static constexpr bool hasBitangent = true;
};

struct UniformBufferLight {
  alignas(16) glm::mat4 projView;
  alignas(16) glm::vec3 pos;
};

struct UniformBufferCamera {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 pos;
};
