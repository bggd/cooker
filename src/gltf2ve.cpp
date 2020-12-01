#include <cassert>
#include <cstdio>
#include <array>
#include <span>
#include <vector>
#include <memory>

#include "cgltf.h"

#include "../external/json/single_include/nlohmann/json.hpp"

struct Geometry {
  std::vector<float> vertexPositions{};
  std::vector<float> vertexColors{};
  std::vector<cgltf_size> indices{};
};

[[nodiscard]] static std::vector<float>* getVertices(Geometry& geo, cgltf_attribute_type attr) {
  if (attr == cgltf_attribute_type_position) {
    return &geo.vertexPositions;
  }
  if (attr == cgltf_attribute_type_color) {
    return &geo.vertexColors;
  }
  return nullptr;
}

[[nodiscard]] static Geometry toDrawArrays(const Geometry& geo) {
  Geometry out;
  out.vertexPositions.reserve(geo.indices.size() * 3);
  out.vertexColors.reserve(geo.indices.size() * 4);

  for (cgltf_size idx : geo.indices) {
    // xyz
    out.vertexPositions.push_back(geo.vertexPositions[idx * 3 + 0]);
    out.vertexPositions.push_back(geo.vertexPositions[idx * 3 + 1]);
    out.vertexPositions.push_back(geo.vertexPositions[idx * 3 + 2]);
    // rgba
    out.vertexColors.push_back(geo.vertexColors[idx * 4 + 0]);
    out.vertexColors.push_back(geo.vertexColors[idx * 4 + 1]);
    out.vertexColors.push_back(geo.vertexColors[idx * 4 + 2]);
    out.vertexColors.push_back(geo.vertexColors[idx * 4 + 3]);
  }

  return out;
}

[[nodiscard]] static nlohmann::json toJson(const Geometry& geo) {
  nlohmann::json jsonVertexPositions(geo.vertexPositions);
  nlohmann::json jsonVertexColors(geo.vertexColors);

  nlohmann::json out = {
      {"vertexPositions", jsonVertexPositions},
      {"vertexColors", jsonVertexColors}};
  return out;
}

int main(int argc, char* argv[]) {
  assert(argc == 2);

  cgltf_options options{};
  cgltf_data* data = nullptr;

  cgltf_result result = cgltf_parse_file(&options, argv[1], &data);
  assert(result == cgltf_result_success);

  result = cgltf_load_buffers(&options, data, argv[1]);
  assert(result == cgltf_result_success);

  result = cgltf_validate(data);
  assert(result == cgltf_result_success);

  Geometry geometry{};
  int numProcessedMesh = 0;

  for (const cgltf_node& node : std::span(data->nodes, data->nodes_count)) {
    if (node.mesh == nullptr) {
      continue;
    }

    assert(!numProcessedMesh); // only one mesh
    numProcessedMesh++;

    for (const cgltf_primitive& primitive : std::span(node.mesh->primitives, node.mesh->primitives_count)) {
      assert(primitive.type == cgltf_primitive_type_triangles);

      for (const cgltf_attribute& attribute : std::span(primitive.attributes, primitive.attributes_count)) {
        if (attribute.type != cgltf_attribute_type_position && attribute.type != cgltf_attribute_type_color) {
          continue;
        }

        const cgltf_accessor* accessor = attribute.data;
        assert(accessor);
        assert(accessor->type == cgltf_type_vec3 || accessor->type == cgltf_type_vec4);

        const cgltf_size elementSize = cgltf_num_components(accessor->type);
        assert(elementSize == 3 || elementSize == 4);

        std::vector<float>* vertices = getVertices(geometry, attribute.type);
        assert(vertices);

        vertices->resize(accessor->count * elementSize);
        const cgltf_size num = cgltf_accessor_unpack_floats(accessor, vertices->data(), accessor->count * elementSize);
        assert(num > 0);
        assert(num == accessor->count * elementSize);
      }

      if (primitive.indices != nullptr) {
        const cgltf_accessor* accessor = primitive.indices;
        assert(accessor->component_type == cgltf_component_type_r_8u || accessor->component_type == cgltf_component_type_r_16u || accessor->component_type == cgltf_component_type_r_32u);
        assert(!accessor->normalized);
        assert(accessor->type == cgltf_type_scalar);

        geometry.indices.resize(accessor->count);
        for (cgltf_size i = 0; i < accessor->count; ++i) {
          geometry.indices[i] = cgltf_accessor_read_index(accessor, i);
        }
      }
    }
  }

  printf("%s\n", toJson(toDrawArrays(geometry)).dump().c_str());

  cgltf_free(data);

  return 0;
}
