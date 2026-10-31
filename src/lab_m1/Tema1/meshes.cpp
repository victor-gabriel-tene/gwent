#include <cmath>
#include <iterator>
#include <vector>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/drawing.h"
#include "lab_m1/Tema1/config/scene.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
using ui::kPi;
}  // namespace

void Tema1::InitMeshes() {
  // Unit shapes are centered at the origin; model matrices set their
  // size/position.
  const std::vector<VertexFormat> rectangle = {
      VertexFormat(glm::vec3(-ui::drawing::kUnitHalfExtent,
                             -ui::drawing::kUnitHalfExtent, 0)),
      VertexFormat(glm::vec3(ui::drawing::kUnitHalfExtent,
                             -ui::drawing::kUnitHalfExtent, 0)),
      VertexFormat(glm::vec3(ui::drawing::kUnitHalfExtent,
                             ui::drawing::kUnitHalfExtent, 0)),
      VertexFormat(glm::vec3(-ui::drawing::kUnitHalfExtent,
                             ui::drawing::kUnitHalfExtent, 0))};
  Mesh* quad = new Mesh(ui::kQuadMesh);
  const std::vector<unsigned int> quad_indices(
      std::begin(ui::drawing::kQuadIndices),
      std::end(ui::drawing::kQuadIndices));
  quad->InitFromData(rectangle, quad_indices);
  AddMeshToList(quad);

  Mesh* triangle = new Mesh(ui::kTriangleMesh);
  triangle->InitFromData(
      {VertexFormat(glm::vec3(0, ui::drawing::kUnitHalfExtent, 0)),
       VertexFormat(glm::vec3(-ui::drawing::kUnitHalfExtent,
                              -ui::drawing::kUnitHalfExtent, 0)),
       VertexFormat(glm::vec3(ui::drawing::kUnitHalfExtent,
                              -ui::drawing::kUnitHalfExtent, 0))},
      std::vector<unsigned int>(std::begin(ui::drawing::kTriangleIndices),
                                std::end(ui::drawing::kTriangleIndices)));
  AddMeshToList(triangle);

  std::vector<VertexFormat> circle = {VertexFormat(glm::vec3(0))};
  std::vector<unsigned int> indices;
  const unsigned int segments = ui::kCircleSegments;

  for (unsigned int i = 0; i < segments; ++i) {
    const float angle = 2 * kPi * i / segments;
    circle.emplace_back(
        glm::vec3(ui::drawing::kUnitHalfExtent * std::cos(angle),
                  ui::drawing::kUnitHalfExtent * std::sin(angle), 0));
    indices.insert(indices.end(), {0, i + 1, (i + 1) % segments + 1});
  }

  Mesh* disk = new Mesh(ui::kCircleMesh);
  disk->InitFromData(circle, indices);
  AddMeshToList(disk);
}

}  // namespace m1
