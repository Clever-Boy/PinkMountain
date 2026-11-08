#pragma once

// Mesh.h — the CPU-side mesh + the OBJ loader (Lesson 15).
//
// A Mesh is the engine's in-memory model of "a drawable thing": an
// interleaved vertex array (position / normal / texcoord — the Lesson-9
// layout, so it drops straight into the existing BufferLayout) plus an
// index array, plus the GPU objects built from them on demand.
//
// WHY CPU-SIDE FIRST: the file format (OBJ) and the GPU format (VBO/VAO)
// are two different shapes of the same data. Parsing produces the CPU
// shape; UploadToGPU() translates it into the GPU shape. Keeping the CPU
// copy around after upload costs memory, but it is what lets us (a) reload
// the file and re-upload without re-parsing from scratch, (b) compute
// normals/tangents/bounds later, and (c) serialize a cooked binary form
// in the Lesson-15 stretch task. Real engines eventually drop the CPU
// copy for shipped builds — but not while you're still iterating.
//
// WHY A HAND-ROLLED OBJ PARSER (and when to stop): OBJ is a line-based
// text format — `v x y z`, `vt u v`, `vn x y z`, `f a/b/c ...` — simple
// enough to parse in ~100 lines, and parsing it yourself teaches the
// format's real quirks (1-based indices! negative relative indices!
// faces that are quads, not triangles!). The moment you want FBX/glTF,
// skeletal animation, or PBR materials, stop and bring in Assimp
// (learnopengl.com/Model-Loading/Model) — writing an FBX parser is a
// career, not a lesson.
//
// OBJ QUIRKS THIS LOADER HANDLES (and the one it refuses):
//   * Faces with N vertices are fan-triangulated: (0,1,2), (0,2,3), ...
//   * Missing UVs / normals: the vertex gets (0,0) / a computed normal.
//   * Negative indices (relative to the end of the list) are resolved.
//   * REFUSED: `usemtl` / .mtl material files. A Mesh carries geometry
//     only; materials are a Lesson-12 Material applied by the caller.
//     (Parsing .mtl is the build task's stretch goal.)
//   * REFUSED: smoothing groups (`s off`). Normals come from the file,
//     or are computed flat per face — never silently averaged.

#include "Pink/Asset/Asset.h"
#include "Pink/Math/PinkMath.h"
#include "Pink/Renderer/Buffer.h"
#include "Pink/Renderer/VertexArray.h"

#include <string>
#include <vector>

namespace Pink {

struct MeshVertex
{
    Vec3 Position{ 0.0f, 0.0f, 0.0f };
    Vec3 Normal{ 0.0f, 1.0f, 0.0f };
    Vec2 TexCoord{ 0.0f, 0.0f };
};

class PINK_API Mesh : public Asset
{
public:
    static constexpr AssetType AssetTypeTag = AssetType::Mesh;

    Mesh(AssetHandle handle, std::string logicalPath)
        : Asset(handle, std::move(logicalPath), AssetType::Mesh) {}

    // Parse an OBJ file from a logical VFS path. Returns false (and logs)
    // on any failure — the caller decides the fallback (error mesh, skip).
    // NOTE: parsing is CPU-only and thread-safe. GPU upload is NOT:
    // call UploadToGPU() on the main thread, after parsing completes.
    bool LoadFromFile(const std::string& logicalPath);

    // Build (or rebuild) the VBO/EBO/VAO from the CPU arrays. Idempotent:
    // calling it twice re-uploads rather than leaking — hot reload's whole
    // trick is "parse again, upload again, same handle".
    void UploadToGPU();

    const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }
    uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_Indices.size()); }
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_Vertices.size()); }

    // Axis-aligned bounds of the CPU data — free, and the frustum-culling
    // lesson (Lesson 11 build task) will want exactly this.
    void ComputeBounds(Vec3& outMin, Vec3& outMax) const;

    bool ReloadFromDisk() override
    {
        if (!LoadFromFile(m_LogicalPath))
            return false;
        UploadToGPU();
        return true;
    }

    const std::vector<MeshVertex>& GetVertices() const { return m_Vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

private:
    std::vector<MeshVertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    Ref<VertexArray> m_VertexArray;
    Ref<VertexBuffer> m_VertexBuffer;
    Ref<IndexBuffer> m_IndexBuffer;
};

// Free function so the AssetManager's loader registry can call it without
// knowing Mesh's constructor signature (see AssetManager.h).
Ref<Mesh> LoadMeshFromFile(const std::string& logicalPath);

}
