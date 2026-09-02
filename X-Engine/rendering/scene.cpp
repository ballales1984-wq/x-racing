#include "rendering/scene.h"
#include "rendering/meshes.h"

namespace xe {

MeshData MeshData::MakeCube() {
    auto v = MakeCubeVertices();
    auto i = MakeCubeIndices();
    MeshData d;
    d.kind = MeshKind::Cube;
    d.vertices.assign(v.begin(), v.end());
    d.indices.assign(i.begin(), i.end());
    return d;
}

MeshData MeshData::MakeTriangle() {
    MeshData d;
    d.kind = MeshKind::Triangle;
    d.vertices = {
        { {  0.0f,  0.25f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };
    d.indices = { 0, 1, 2 };
    return d;
}

MeshData MeshData::MakeQuad() {
    MeshData d;
    d.kind = MeshKind::Quad;
    d.vertices = {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    };
    d.indices = { 0, 1, 2,  0, 2, 3 };
    return d;
}

MeshData MeshData::MakeTexturedQuad() {
    MeshData d;
    d.kind = MeshKind::Quad;
    d.vertices = {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    };
    d.indices = { 0, 1, 2,  0, 2, 3 };
    return d;
}

}  // namespace xe