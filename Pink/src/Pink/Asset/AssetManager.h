#pragma once

// AssetManager.h — the registry, the async loader, and the hot reloader
// (Lesson 15).
//
// THREE JOBS, ONE CLASS — and the discipline is keeping them separate:
//
//   1. REGISTRY: one Ref<Asset> per handle. Load the same path twice
//      and you get the SAME object back (no duplicate GPU uploads, no
//      duplicate VRAM). Unload is explicit and rare — in practice the
//      manager owns everything until shutdown, which is fine: a game's
//      asset set is bounded, and "load everything at level start, free
//      everything at level end" beats per-asset refcounting for a first
//      engine. (Reference counting across the registry is the
//      Lesson-15 stretch task.)
//
//   2. ASYNC LOADING: the frame must never stall on the disk. The rule
//      that makes this safe is the CPU/GPU SPLIT —
//        worker thread: read bytes + parse/decode (NO GL calls)
//        main thread:  create GPU objects + insert into the registry
//      The manager enforces the split by construction: the worker's
//      std::future carries a CPU-only object, and the main thread's
//      OnUpdate() is the ONLY place that calls UploadToGPU() and the
//      only place that touches the registry. There is no code path by
//      which a worker thread can reach a GL call. If you remember one
//      thing from this lesson, remember that sentence.
//
//   3. HOT RELOAD: a background watcher thread polls last-write times
//      (250 ms — cheap, no OS file-watch API needed, portable). On a
//      change it does NOT reload: it only ENQUEUES the handle. The main
//      thread's OnUpdate() performs the actual reload, for the same
//      GL-thread reason as (2). A broken edit is safe: ReloadFromDisk()
//      parses first and only swaps GPU data on success (the Lesson-8
//      invariant, generalized).
//
// THREADING CONTRACT (write it on the whiteboard):
//   * Registry + completion queues: touched ONLY on the main thread,
//     EXCEPT the watcher's reload queue, which is mutex-protected.
//   * Watcher thread: reads file mtimes only. Never parses, never
//     touches GL, never touches the registry.
//   * Worker threads (std::async): parse/decode only. They receive a
//     path string and return a CPU-populated asset. They never see a
//     GL object, a VAO, or the registry.
//   * OnShutdown joins the watcher BEFORE the registry is destroyed.
//     A thread touching a dead registry is the classic shutdown crash.

#include "Pink/Asset/Asset.h"
#include "Pink/Core/SubSystem.h"
#include "Pink/Renderer/Mesh.h"
#include "Pink/Asset/TextureAsset.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Pink {

class PINK_API AssetManager : public SubSystem
{
public:
    using OnLoadedCallback = std::function<void(Ref<Asset>)>;

    AssetManager() = default;
    ~AssetManager() override { /* OnShutdown does the real work */ }

    int GetInitPriority() const override { return -50; } // after MemoryService (-100), before renderers

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnShutdown() override;

    // --- Synchronous loading ------------------------------------------------
    // Returns the cached asset if present, else loads it NOW on the
    // calling thread. Use at level load / startup. NEVER per frame —
    // a synchronous load is a frame hitch by definition.
    template<typename T>
    Ref<T> Load(const std::string& logicalPath)
    {
        AssetHandle handle = HashAssetPath(logicalPath);
        if (auto cached = Get<T>(handle))
            return cached;
        Ref<Asset> asset = LoadByType(T::AssetTypeTag, logicalPath);
        if (!asset)
            return nullptr;
        m_Registry[handle] = asset;
        return std::dynamic_pointer_cast<T>(asset);
    }

    // --- Asynchronous loading ----------------------------------------------
    // Parse/decode on a worker thread; GPU upload + registry insert +
    // callback on the main thread (next OnUpdate). The callback receives
    // nullptr on failure. The returned handle is valid IMMEDIATELY —
    // Get<T>(handle) returns nullptr until the load completes, so callers
    // can poll or just wait for the callback.
    AssetHandle LoadMeshAsync(const std::string& logicalPath, OnLoadedCallback onLoaded = nullptr);
    AssetHandle LoadTextureAsync(const std::string& logicalPath, OnLoadedCallback onLoaded = nullptr);

    // --- Lookup --------------------------------------------------------------
    template<typename T>
    Ref<T> Get(AssetHandle handle) const
    {
        auto it = m_Registry.find(handle);
        if (it == m_Registry.end())
            return nullptr;
        return std::dynamic_pointer_cast<T>(it->second);
    }

    Ref<Asset> Get(AssetHandle handle) const
    {
        auto it = m_Registry.find(handle);
        return it == m_Registry.end() ? nullptr : it->second;
    }

    bool IsLoaded(AssetHandle handle) const { return m_Registry.count(handle) != 0; }

    // --- Hot reload -----------------------------------------------------------
    // Register a path for watching (meshes/textures loaded through the
    // manager are watched automatically). A shader path can be registered
    // too — but shaders keep their Lesson-8 reload path; see the build
    // task for unifying the two.
    void WatchForReload(AssetHandle handle, const std::string& logicalPath);

    // --- Loader registry ------------------------------------------------------
    // New asset types plug in here instead of editing the manager:
    //   RegisterLoader(AssetType::Material, [](path){ ... });
    // Load() dispatches through this table.
    using LoaderFn = std::function<Ref<Asset>(const std::string& logicalPath)>;
    void RegisterLoader(AssetType type, LoaderFn loader) { m_Loaders[type] = std::move(loader); }

private:
    Ref<Asset> LoadByType(AssetType type, const std::string& logicalPath);

    // Main-thread completion processing (called from OnUpdate).
    void ProcessCompletions();
    void ProcessReloadRequests();

    // The watcher thread body. Reads mtimes, enqueues handles. NOTHING else.
    void WatcherLoop();

private:
    std::unordered_map<AssetHandle, Ref<Asset>> m_Registry;
    std::unordered_map<AssetType, LoaderFn> m_Loaders;

    // --- Async completions (main thread only) --------------------------------
    struct PendingLoad
    {
        std::future<Ref<Asset>> Future;              // CPU stage result
        std::function<void(Ref<Asset>)> Finalize;    // main-thread GPU upload
        OnLoadedCallback OnLoaded;
    };
    std::vector<PendingLoad> m_PendingLoads;

    // --- Hot reload ------------------------------------------------------------
    std::unordered_map<AssetHandle, std::filesystem::file_time_type> m_WatchedTimes;
    std::unordered_map<AssetHandle, std::string> m_WatchedPaths;
    std::vector<AssetHandle> m_ReloadQueue; // written by watcher, read by main
    std::mutex m_ReloadMutex;

    std::thread m_WatcherThread;
    std::atomic<bool> m_WatcherRunning{ false };
};

}
