// AssetManager.cpp — registry, async completions, watcher thread (Lesson 15).
//
// READ THIS FILE TOP TO BOTTOM once: it is short, and the threading
// contract from the header is enforced here in about six places. Every
// one of them is a place where a naive implementation silently breaks:
// a GL call on a worker, a registry write from two threads, a watcher
// thread that outlives the manager.

#include "Pink/Asset/AssetManager.h"
#include "Pink/Core/Log.h"
#include "Pink/Core/VirtualFileSystem.h"

namespace Pink {

void AssetManager::OnInit()
{
    // Built-in loaders. Mesh: CPU parse here would be the sync path —
    // note LoadMeshFromFile() uploads to the GPU, so it is ONLY valid
    // on the main thread. The async path never calls it (see
    // LoadMeshAsync: parse on worker, upload in the Finalize lambda).
    RegisterLoader(AssetType::Mesh, [](const std::string& path)
    {
        return std::static_pointer_cast<Asset>(LoadMeshFromFile(path));
    });
    RegisterLoader(AssetType::Texture, [](const std::string& path)
    {
        auto tex = CreateRef<TextureAsset>(HashAssetPath(path), path);
        if (!tex->DecodeFromFile(path))
            return Ref<Asset>{};
        if (!tex->UploadToGPU()) // sync path runs on the main thread: legal
            return Ref<Asset>{};
        return std::static_pointer_cast<Asset>(tex);
    });

    m_WatcherRunning = true;
    m_WatcherThread = std::thread(&AssetManager::WatcherLoop, this);
    PM_CORE_INFO("AssetManager: initialized (hot-reload watcher running)");
}

void AssetManager::OnUpdate(float /*deltaTime*/)
{
    ProcessCompletions();
    ProcessReloadRequests();
}

void AssetManager::OnShutdown()
{
    // THE ORDER MATTERS: stop the watcher FIRST, join it, and only then
    // clear the registry. A watcher polling mtimes of assets that no
    // longer exist is a use-after-free with a thread attached.
    m_WatcherRunning = false;
    if (m_WatcherThread.joinable())
        m_WatcherThread.join();

    m_PendingLoads.clear();
    size_t released = m_Registry.size();
    m_Registry.clear();
    m_WatchedTimes.clear();
    m_WatchedPaths.clear();
    PM_CORE_INFO("AssetManager: shut down ({} assets released)", released);
}

Ref<Asset> AssetManager::LoadByType(AssetType type, const std::string& logicalPath)
{
    auto it = m_Loaders.find(type);
    if (it == m_Loaders.end())
    {
        PM_CORE_ERROR("AssetManager: no loader registered for asset type {}", static_cast<int>(type));
        return nullptr;
    }
    Ref<Asset> asset = it->second(logicalPath);
    if (asset)
        WatchForReload(asset->GetHandle(), logicalPath);
    return asset;
}

AssetHandle AssetManager::LoadMeshAsync(const std::string& logicalPath, OnLoadedCallback onLoaded)
{
    AssetHandle handle = HashAssetPath(logicalPath);
    if (auto cached = Get<Mesh>(handle))
    {
        if (onLoaded)
            onLoaded(cached); // already loaded: callback immediately, same thread
        return handle;
    }

    // WORKER THREAD: parse only. No GL, no registry, no VFS writes —
    // just bytes in, CPU mesh out. std::launch::async FORCES a new
    // thread; the default policy may defer to the first .get() call,
    // which would silently turn this "async" load into a main-thread
    // hitch. That default-policy trap is the #1 std::async footgun.
    auto future = std::async(std::launch::async, [logicalPath]()
    {
        auto mesh = CreateRef<Mesh>(HashAssetPath(logicalPath), logicalPath);
        if (!mesh->LoadFromFile(logicalPath))
            return Ref<Asset>{};
        return std::static_pointer_cast<Asset>(mesh);
    });

    // MAIN THREAD (later, in ProcessCompletions): upload + register.
    PendingLoad pending;
    pending.Future = std::move(future);
    pending.Finalize = [](Ref<Asset> asset)
    {
        std::static_pointer_cast<Mesh>(asset)->UploadToGPU();
    };
    pending.OnLoaded = std::move(onLoaded);
    m_PendingLoads.push_back(std::move(pending));

    WatchForReload(handle, logicalPath);
    return handle;
}

AssetHandle AssetManager::LoadTextureAsync(const std::string& logicalPath, OnLoadedCallback onLoaded)
{
    AssetHandle handle = HashAssetPath(logicalPath);
    if (auto cached = Get<TextureAsset>(handle))
    {
        if (onLoaded)
            onLoaded(cached);
        return handle;
    }

    auto future = std::async(std::launch::async, [logicalPath]()
    {
        auto tex = CreateRef<TextureAsset>(HashAssetPath(logicalPath), logicalPath);
        if (!tex->DecodeFromFile(logicalPath))
            return Ref<Asset>{};
        return std::static_pointer_cast<Asset>(tex);
    });

    PendingLoad pending;
    pending.Future = std::move(future);
    pending.Finalize = [](Ref<Asset> asset)
    {
        std::static_pointer_cast<TextureAsset>(asset)->UploadToGPU();
    };
    pending.OnLoaded = std::move(onLoaded);
    m_PendingLoads.push_back(std::move(pending));

    WatchForReload(handle, logicalPath);
    return handle;
}

void AssetManager::ProcessCompletions()
{
    // Poll, never block: wait_for(0s) returns immediately. A load that
    // isn't ready yet simply stays in the list for next frame — the
    // frame rate is never held hostage to the disk.
    for (auto it = m_PendingLoads.begin(); it != m_PendingLoads.end();)
    {
        if (it->Future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            ++it;
            continue;
        }
        Ref<Asset> asset = it->Future.get(); // get() on a ready future: no block
        if (asset)
        {
            it->Finalize(asset); // MAIN THREAD: the only place GL is touched
            m_Registry[asset->GetHandle()] = asset;
            PM_CORE_INFO("AssetManager: async load complete: '{}'", asset->GetLogicalPath());
        }
        else
        {
            PM_CORE_ERROR("AssetManager: async load FAILED (see earlier errors)");
        }
        if (it->OnLoaded)
            it->OnLoaded(asset); // nullptr on failure — the callback decides the fallback
        it = m_PendingLoads.erase(it);
    }
}

void AssetManager::WatchForReload(AssetHandle handle, const std::string& logicalPath)
{
    auto t = VirtualFileSystem::LastWriteTime(logicalPath);
    if (!t)
        return; // unresolvable path: nothing to watch
    // The watcher thread reads these maps concurrently — the mutex
    // protects both sides. Forgetting this lock is a textbook data race
    // that only shows up as a once-a-week crash.
    std::lock_guard<std::mutex> lock(m_ReloadMutex);
    m_WatchedPaths[handle] = logicalPath;
    m_WatchedTimes[handle] = *t;
}

void AssetManager::WatcherLoop()
{
    using namespace std::chrono_literals;
    while (m_WatcherRunning)
    {
        std::this_thread::sleep_for(250ms);
        if (!m_WatcherRunning)
            break;
        // Snapshot under the mutex, compare outside it: mtime reads are
        // the only filesystem work done here, and they never touch the
        // registry — the watcher owns NO asset state.
        std::vector<std::pair<AssetHandle, std::string>> changed;
        {
            std::lock_guard<std::mutex> lock(m_ReloadMutex);
            for (const auto& [handle, path] : m_WatchedPaths)
            {
                auto t = VirtualFileSystem::LastWriteTime(path);
                if (t && *t != m_WatchedTimes[handle])
                {
                    m_WatchedTimes[handle] = *t;
                    changed.emplace_back(handle, path);
                }
            }
        }
        if (!changed.empty())
        {
            std::lock_guard<std::mutex> lock(m_ReloadMutex);
            for (const auto& [handle, path] : changed)
            {
                (void)path;
                m_ReloadQueue.push_back(handle);
            }
        }
    }
}

void AssetManager::ProcessReloadRequests()
{
    std::vector<AssetHandle> requests;
    {
        std::lock_guard<std::mutex> lock(m_ReloadMutex);
        requests.swap(m_ReloadQueue); // drain atomically; watcher keeps appending
    }
    for (AssetHandle handle : requests)
    {
        auto it = m_Registry.find(handle);
        if (it == m_Registry.end())
            continue; // asset was never loaded / already unloaded
        if (it->second->ReloadFromDisk())
            PM_CORE_INFO("AssetManager: hot-reloaded '{}'", it->second->GetLogicalPath());
        else
            PM_CORE_ERROR("AssetManager: hot reload FAILED for '{}' — old version kept",
                          it->second->GetLogicalPath());
    }
}

}
