// Mesh.cpp — the OBJ parser + GPU upload (Lesson 15).
//
// THE PARSER'S ONE HARD RULE: OBJ indices are 1-BASED, and a face
// references THREE separate lists (v / vt / vn) with three separate
// index spaces. A face token "5/2/7" means: position #5, texcoord #2,
// normal #7. The GPU wants ONE index space (one vertex = one
// position+normal+uv tuple), so the loader must DEDUPLICATE: the same
// (pos, uv, normal) triple appearing in two faces must map to the SAME
// vertex, or the index buffer explodes and the mesh renders with seams.
//
// Deduplication is a hash map from triple -> vertex index. That map is
// the whole reason the loader is O(n) instead of O(n^2) — the naive
// "scan the vertex list for a match" version is the classic beginner
// trap, and it turns a 100k-vertex model into a multi-second load.

#include "Pink/Renderer/Mesh.h"
#include "Pink/Core/Log.h"
#include "Pink/Core/VirtualFileSystem.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace Pink {

namespace {

// One face-vertex reference: indices into the three OBJ lists.
// -1 = absent (e.g. "f 1//3" has no texcoord).
struct ObjIndex
{
    int Position = -1;
    int TexCoord = -1;
    int Normal = -1;

    bool operator==(const ObjIndex& other) const
    {
        return Position == other.Position && TexCoord == other.TexCoord && Normal == other.Normal;
    }
};

struct ObjIndexHash
{
    size_t operator()(const ObjIndex& i) const
    {
        // Small-integer hash combine; indices are < 2^31 in practice.
        size_t h = static_cast<size_t>(i.Position + 1) * 73856093u;
        h ^= static_cast<size_t>(i.TexCoord + 1) * 19349663u;
        h ^= static_cast<size_t>(i.Normal + 1) * 83492791u;
        return h;
    }
};

// OBJ indices are 1-based, or negative (relative to the end of the list:
// -1 = the last element so far). Convert to a 0-based index into a list
// of `count` elements; returns -1 when the token was absent.
int ResolveObjIndex(const std::string& token, size_t count)
{
    if (token.empty())
        return -1;
    int i = std::stoi(token);
    if (i > 0)
        return i - 1;
    if (i < 0)
        return static_cast<int>(count) + i; // -1 -> count-1
    return -1; // OBJ has no index 0 — a malformed file
}

ObjIndex ParseFaceToken(const std::string& token, size_t posCount, size_t uvCount, size_t normalCount)
{
    ObjIndex out;
    // Split on '/': "v", "v/vt", "v//vn", "v/vt/vn".
    size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos)
    {
        out.Position = ResolveObjIndex(token, posCount);
        return out;
    }
    out.Position = ResolveObjIndex(token.substr(0, firstSlash), posCount);

    size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos)
    {
        out.TexCoord = ResolveObjIndex(token.substr(firstSlash + 1), uvCount);
        return out;
    }
    out.TexCoord = ResolveObjIndex(token.substr(firstSlash + 1, secondSlash - firstSlash - 1), uvCount);
    out.Normal = ResolveObjIndex(token.substr(secondSlash + 1), normalCount);
    return out;
}

} // namespace

bool Mesh::LoadFromFile(const std::string& logicalPath)
{
    std::string text;
    if (!VirtualFileSystem::ReadFileText(logicalPath, text))
    {
        PM_CORE_ERROR("Mesh: cannot read '{}'", logicalPath);
        return false;
    }

    std::vector<Vec3> positions;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;

    std::vector<ObjIndex> faceVerts; // all faces, flattened, in order
    std::vector<uint32_t> faceSizes; // vertices per face (3, 4, ...)

    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream ls(line);
        std::string tag;
        if (!(ls >> tag))
            continue; // blank line
        if (tag[0] == '#')
            continue; // comment

        if (tag == "v")
        {
            Vec3 p;
            ls >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (tag == "vt")
        {
            Vec2 uv;
            ls >> uv.x >> uv.y;
            texcoords.push_back(uv);
        }
        else if (tag == "vn")
        {
            Vec3 n;
            ls >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (tag == "f")
        {
            // Read the whole face, then fan-triangulate it below.
            size_t start = faceVerts.size();
            std::string token;
            while (ls >> token)
                faceVerts.push_back(ParseFaceToken(token, positions.size(), texcoords.size(), normals.size()));
            faceSizes.push_back(static_cast<uint32_t>(faceVerts.size() - start));
        }
        // Deliberately ignored: o, g, s, usemtl, mtllib (see Mesh.h).
    }

    // --- Build the deduplicated vertex/index arrays ----------------------
    m_Vertices.clear();
    m_Indices.clear();
    std::unordered_map<ObjIndex, uint32_t, ObjIndexHash> seen;
    seen.reserve(faceVerts.size());

    size_t cursor = 0;
    for (uint32_t faceSize : faceSizes)
    {
        if (faceSize < 3)
        {
            PM_CORE_WARN("Mesh '{}': degenerate face ({} verts) skipped", logicalPath, faceSize);
            cursor += faceSize;
            continue;
        }
        // Fan triangulation: (0,1,2), (0,2,3), (0,3,4), ...
        auto emit = [&](size_t k)
        {
            const ObjIndex& idx = faceVerts[cursor + k];
            auto it = seen.find(idx);
            if (it != seen.end())
            {
                m_Indices.push_back(it->second);
                return;
            }
            MeshVertex v;
            // Defensive: a malformed file can reference past the end.
            if (idx.Position >= 0 && idx.Position < static_cast<int>(positions.size()))
                v.Position = positions[idx.Position];
            if (idx.TexCoord >= 0 && idx.TexCoord < static_cast<int>(texcoords.size()))
                v.TexCoord = texcoords[idx.TexCoord];
            if (idx.Normal >= 0 && idx.Normal < static_cast<int>(normals.size()))
                v.Normal = normals[idx.Normal];
            uint32_t newIndex = static_cast<uint32_t>(m_Vertices.size());
            m_Vertices.push_back(v);
            seen[idx] = newIndex;
            m_Indices.push_back(newIndex);
        };
        for (uint32_t k = 1; k + 1 < faceSize; ++k)
        {
            emit(0);
            emit(k);
            emit(k + 1);
        }
        cursor += faceSize;
    }

    // --- Normals: compute flat face normals where the file has none -----
    // Flat shading needs one normal PER FACE — but dedup shares vertices
    // between faces (the (pos,uv,-1) triple is identical on both sides of
    // an edge). Writing the face normal into a shared vertex would give
    // it the LAST face's normal — wrong. So expand: one vertex per
    // triangle corner, each carrying its own face normal. (For a cube
    // this means 36 verts instead of 8 — that IS the cost of flat shading
    // without file normals, and it's why cube.obj ships normals.)
    if (normals.empty() && !m_Indices.empty())
    {
        std::vector<MeshVertex> flatVerts;
        std::vector<uint32_t> flatIndices;
        flatVerts.reserve(m_Indices.size());
        flatIndices.reserve(m_Indices.size());
        for (size_t i = 0; i + 2 < m_Indices.size(); i += 3)
        {
            MeshVertex a = m_Vertices[m_Indices[i + 0]];
            MeshVertex b = m_Vertices[m_Indices[i + 1]];
            MeshVertex c = m_Vertices[m_Indices[i + 2]];
            Vec3 n = Normalize(Cross(b.Position - a.Position, c.Position - a.Position));
            a.Normal = b.Normal = c.Normal = n;
            uint32_t base = static_cast<uint32_t>(flatVerts.size());
            flatVerts.push_back(a); flatVerts.push_back(b); flatVerts.push_back(c);
            flatIndices.push_back(base + 0); flatIndices.push_back(base + 1); flatIndices.push_back(base + 2);
        }
        m_Vertices = std::move(flatVerts);
        m_Indices = std::move(flatIndices);
    }

    PM_CORE_INFO("Mesh: '{}' — {} verts, {} indices ({} faces)",
                 logicalPath, m_Vertices.size(), m_Indices.size(), faceSizes.size());
    return true;
}

void Mesh::UploadToGPU()
{
    if (m_Vertices.empty() || m_Indices.empty())
    {
        PM_CORE_WARN("Mesh '{}': nothing to upload", m_LogicalPath);
        return;
    }

    // Interleave into the Lesson-9 layout: Float3 pos, Float3 normal,
    // Float2 uv — 8 floats per vertex, the SAME layout the Sandbox's
    // procedural cube used, so the existing shaders need no changes.
    std::vector<float> interleaved;
    interleaved.reserve(m_Vertices.size() * 8);
    for (const MeshVertex& v : m_Vertices)
    {
        interleaved.push_back(v.Position.x); interleaved.push_back(v.Position.y); interleaved.push_back(v.Position.z);
        interleaved.push_back(v.Normal.x);   interleaved.push_back(v.Normal.y);   interleaved.push_back(v.Normal.z);
        interleaved.push_back(v.TexCoord.x); interleaved.push_back(v.TexCoord.y);
    }

    // Re-upload path (hot reload): creating fresh buffers and dropping the
    // old Refs releases the old GL objects via their destructors. The
    // VertexArray is rebuilt, not mutated — mutating a live VAO mid-frame
    // is how you get a one-frame glitch; replacing it is atomic.
    m_VertexBuffer = VertexBuffer::Create(interleaved.data(), static_cast<uint32_t>(interleaved.size() * sizeof(float)));
    BufferLayout layout = {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float3, "a_Normal" },
        { ShaderDataType::Float2, "a_TexCoord" }
    };
    m_VertexBuffer->SetLayout(layout); // BEFORE AddVertexBuffer — Lesson 9 invariant

    m_VertexArray = VertexArray::Create();
    m_VertexArray->AddVertexBuffer(m_VertexBuffer);

    m_IndexBuffer = IndexBuffer::Create(m_Indices.data(), static_cast<uint32_t>(m_Indices.size()));
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);
}

void Mesh::ComputeBounds(Vec3& outMin, Vec3& outMax) const
{
    if (m_Vertices.empty())
    {
        outMin = outMax = Vec3{ 0.0f, 0.0f, 0.0f };
        return;
    }
    outMin = outMax = m_Vertices[0].Position;
    for (const MeshVertex& v : m_Vertices)
    {
        outMin.x = std::min(outMin.x, v.Position.x);
        outMin.y = std::min(outMin.y, v.Position.y);
        outMin.z = std::min(outMin.z, v.Position.z);
        outMax.x = std::max(outMax.x, v.Position.x);
        outMax.y = std::max(outMax.y, v.Position.y);
        outMax.z = std::max(outMax.z, v.Position.z);
    }
}

Ref<Mesh> LoadMeshFromFile(const std::string& logicalPath)
{
    auto mesh = CreateRef<Mesh>(HashAssetPath(logicalPath), logicalPath);
    if (!mesh->LoadFromFile(logicalPath))
        return nullptr;
    mesh->UploadToGPU(); // NOTE: main thread only — GL calls are not thread-safe
    return mesh;
}

}
