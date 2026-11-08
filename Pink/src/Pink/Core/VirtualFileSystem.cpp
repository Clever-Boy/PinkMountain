// VirtualFileSystem.cpp — mount resolution + whole-file reads (Lesson 15).
//
// Two rules worth internalizing:
//
//  1. A mount prefix is a PATH PREFIX, not a string prefix. "assets" must
//     match "assets/shaders/x.glsl" but NOT "assets2/x.glsl". We enforce
//     that by comparing path COMPONENTS, not characters.
//  2. Resolve() never throws. std::filesystem calls are wrapped in
//     std::error_code overloads: a missing file is a normal outcome
//     (std::nullopt), not an exception — loading is a hot path and the
//     caller decides what "missing" means.

#include "Pink/Core/VirtualFileSystem.h"
#include "Pink/Core/Log.h"

#include <algorithm>
#include <fstream>

namespace Pink {

void VirtualFileSystem::Mount(const std::string& virtualPrefix, const std::string& physicalPath)
{
    std::error_code ec;
    std::filesystem::path physical = std::filesystem::absolute(physicalPath, ec);
    if (ec)
    {
        PM_CORE_WARN("VFS: could not absolutize mount '{}' — skipping", physicalPath);
        return;
    }
    if (!std::filesystem::is_directory(physical, ec))
    {
        // Warning, not error: a DLC/mod mount may legitimately be absent.
        PM_CORE_WARN("VFS: mount directory '{}' does not exist (prefix '{}')", physicalPath, virtualPrefix);
    }
    s_Mounts.push_back({ virtualPrefix, physical });
    PM_CORE_INFO("VFS: mounted '{}' at '{}'", physicalPath, virtualPrefix.empty() ? "<root>" : virtualPrefix);
}

std::optional<std::filesystem::path> VirtualFileSystem::Resolve(const std::string& logicalPath)
{
    std::filesystem::path logical(logicalPath);

    // Last mounted = highest priority (reverse iteration).
    for (auto it = s_Mounts.rbegin(); it != s_Mounts.rend(); ++it)
    {
        std::filesystem::path candidate = it->PhysicalPath;
        if (!it->VirtualPrefix.empty())
        {
            // Component-wise prefix match: strip the prefix from the
            // logical path only if it matches whole components.
            std::filesystem::path prefix(it->VirtualPrefix);
            auto lit = logical.begin();
            bool matches = true;
            for (auto pit = prefix.begin(); pit != prefix.end(); ++pit, ++lit)
            {
                if (lit == logical.end() || *lit != *pit) { matches = false; break; }
            }
            if (!matches)
                continue; // this mount's prefix doesn't apply to this path
            for (; lit != logical.end(); ++lit)
                candidate /= *lit;
        }
        else
        {
            candidate /= logical;
        }

        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec))
            return candidate;
    }
    return std::nullopt;
}

bool VirtualFileSystem::ReadFileBinary(const std::string& logicalPath, std::vector<uint8_t>& out)
{
    auto resolved = Resolve(logicalPath);
    if (!resolved)
    {
        PM_CORE_WARN("VFS: file not found: '{}'", logicalPath);
        return false;
    }
    std::ifstream in(*resolved, std::ios::binary | std::ios::ate);
    if (!in)
        return false;
    std::streamsize size = in.tellg();
    if (size < 0)
        return false;
    out.resize(static_cast<size_t>(size));
    in.seekg(0, std::ios::beg);
    return static_cast<bool>(in.read(reinterpret_cast<char*>(out.data()), size));
}

bool VirtualFileSystem::ReadFileText(const std::string& logicalPath, std::string& out)
{
    std::vector<uint8_t> bytes;
    if (!ReadFileBinary(logicalPath, bytes))
        return false;
    out.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

std::optional<std::filesystem::file_time_type> VirtualFileSystem::LastWriteTime(const std::string& logicalPath)
{
    auto resolved = Resolve(logicalPath);
    if (!resolved)
        return std::nullopt;
    std::error_code ec;
    auto t = std::filesystem::last_write_time(*resolved, ec);
    if (ec)
        return std::nullopt;
    return t;
}

void VirtualFileSystem::ClearMounts()
{
    s_Mounts.clear();
}

}
