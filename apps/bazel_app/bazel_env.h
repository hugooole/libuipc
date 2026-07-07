#pragma once
// Bazel-only runtime environment helper for the example programs.
//
// The CMake/xmake examples rely on the `apps/app` library (AssetDir,
// test::Scene) whose paths are baked in at compile time as absolute paths.
// Those cannot exist under Bazel's sandbox/runfiles model, so the Bazel
// example targets use their own main (main_bazel.cpp) plus this helper, which
// resolves the backend module directory, data assets, and an output directory
// through Bazel runfiles instead of compile-time macros.
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <uipc/common/json.h>

#include "tools/cpp/runfiles/runfiles.h"

namespace uipc::bazel_env
{
namespace fs = std::filesystem;
using bazel::tools::cpp::runfiles::Runfiles;

/// Owns the runfiles handle and resolves paths relative to the workspace root.
class Env
{
  public:
    explicit Env(const char* argv0)
    {
        std::string error;
        m_runfiles.reset(Runfiles::Create(argv0, &error));
        if(!m_runfiles)
            throw std::runtime_error("Failed to init runfiles: " + error);
    }

    /// Absolute path of a runfile given its workspace-relative path
    /// (e.g. "libuipc/assets/sim_data/tetmesh/cube.msh").
    std::string rlocation(const std::string& workspace_relative) const
    {
        auto p = m_runfiles->Rlocation(workspace_relative);
        if(p.empty())
            throw std::runtime_error("Runfile not found: " + workspace_relative);
        return p;
    }

    /// Directory that uipc::init() expects as "module_dir": it must contain all
    /// loadable modules (backend + sanity_check) side by side. Under Bazel these
    /// live in different runfiles subdirs (cc_shared_library places each in its
    /// own package dir), so stage a single directory of symlinks to them.
    std::string module_dir() const
    {
        auto staged = fs::current_path() / "uipc_modules";
        fs::create_directories(staged);
        for(const char* rel : {"libuipc/src/libuipc_backend_cuda.so",
                               "libuipc/src/sanity_check/libuipc_sanity_check.so"})
        {
            auto src = m_runfiles->Rlocation(rel);  // empty if not a data dep
            if(src.empty())
                continue;
            auto dst = staged / fs::path(src).filename();
            std::error_code ec;
            fs::remove(dst, ec);
            fs::create_symlink(src, dst, ec);
        }
        return staged.string();
    }

    /// Directory (with trailing separator) of a runfile, e.g. the folder that
    /// holds a scene .json next to the example.
    std::string folder_of(const std::string& workspace_relative) const
    {
        return (fs::path(rlocation(workspace_relative)).parent_path() / "").string();
    }

    /// A writable output directory under the current working directory.
    std::string output_dir(std::string_view name) const
    {
        auto p = fs::current_path() / name;
        fs::create_directories(p);
        return (p / "").string();
    }

  private:
    std::unique_ptr<Runfiles> m_runfiles;
};

/// Small standalone replacement for test::Scene::dump_config.
inline void dump_config(const Json& config, std::string_view workspace)
{
    std::ofstream ofs(std::string{workspace} + "config.json");
    ofs << config.dump(4);
}
}  // namespace uipc::bazel_env
