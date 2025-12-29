#include "uipc/common/exception.h"
#include "uipc/common/json.h"
#include <filesystem>
#include <uipc/uipc.h>
#include <uipc/constitution/affine_body_constitution.h>

#include "tools/cpp/runfiles/runfiles.h"
#include "uipc/common/logger.h"

using bazel::tools::cpp::runfiles::Runfiles;

std::string find_module_dir(const char* argv0)
{
    std::string error;
    auto runfiles = Runfiles::Create(argv0, &error);
    if (!runfiles)
        throw std::runtime_error(error);

    // workspace_name/path/to/file
    auto so_path = runfiles->Rlocation(
        "libuipc/src/libuipc_backend_cuda.so");

    return std::filesystem::path(so_path).parent_path().string();
}

int main(int argc, char** argv)
{
    using namespace uipc;
    using namespace uipc::core;
    using namespace uipc::geometry;
    using namespace uipc::constitution;
    namespace fs = std::filesystem;

    logger::set_level(Logger::Level::debug);
    Json uipc_config = Json::object();
    uipc_config["module_dir"] = find_module_dir(argv[0]);
    logger::info("(ZHIGUO)module_dir: {}", uipc_config["module_dir"].get<std::string>());
    init(uipc_config);

    // try {
    // Engine engine{"none"};
    Engine engine{"cuda"};

    World world{engine};
    auto  config      = Scene::default_config();
    config["sanity_check"]["enable"] = false;
    config["gravity"] = Vector3{0, -9.8, 0};
    config["dt"]      = 0.01_s;

    Scene scene{config};
    {
        // create constitution and contact model
        Json abd_config    = AffineBodyConstitution::default_config();
        abd_config["name"] = "ARAP";
        AffineBodyConstitution abd {abd_config};
        scene.constitution_tabular().insert(abd);

        // friction ratio and contact resistance
        scene.contact_tabular().default_model(0.5, 1.0_GPa);
        auto default_element = scene.contact_tabular().default_element();

        // create a regular tetrahedron
        vector<Vector3>  Vs = {Vector3{0, 1, 0},
                               Vector3{0, 0, 1},
                               Vector3{-std::sqrt(3) / 2, 0, -0.5},
                               Vector3{std::sqrt(3) / 2, 0, -0.5}};
        vector<Vector4i> Ts = {Vector4i{0, 1, 2, 3}};

        // setup a base mesh to reduce the later work
        SimplicialComplex base_mesh = tetmesh(Vs, Ts);
        // apply the constitution and contact model to the base mesh
        abd.apply_to(base_mesh, 100.0_MPa);
        // apply the default contact model to the base mesh
        default_element.apply_to(base_mesh);

        // label the surface, enable the contact
        label_surface(base_mesh);
        // label the triangle orientation to export the correct surface mesh
        label_triangle_orient(base_mesh);

        SimplicialComplex mesh1 = base_mesh;
        {
            // move the mesh1 up for 1 unit
            auto pos_view = view(mesh1.positions());
            std::ranges::transform(pos_view,
                                   pos_view.begin(),
                                   [](const Vector3& v) -> Vector3
                                   { return v + Vector3::UnitY() * 1.3; });
        }

        SimplicialComplex mesh2 = base_mesh;
        {
            // find the is_fixed attribute
            auto is_fixed = mesh2.instances().find<IndexT>(builtin::is_fixed);
            // set the first instance to be fixed
            auto is_fixed_view = view(*is_fixed);
            is_fixed_view[0]   = 1;
        }


        // create object with two geometries
        auto object = scene.objects().create("tets");
        {
            object->geometries().create(mesh1);
            object->geometries().create(mesh2);
        }
    }

    logger::info("(ZHIGUO)world.init(scene) start");
    world.init(scene);
    // logger::info("(ZHIGUO)world.init(scene) done");

    // SceneIO sio{scene};

    // auto this_output_path = fs::path("/home/ps/Projects/libuipc/output/examples/hello_affine_body/");

    // sio.write_surface(fmt::format("{}scene_surface{}.obj", this_output_path.string(), 0));

    // for(int i = 1; i < 50; i++)
    // {
    //     world.advance();
    //     world.sync();
    //     world.retrieve();
    //     sio.write_surface(fmt::format("{}scene_surface{}.obj", this_output_path.string(), i));
    // }
    // } catch (const uipc::Exception& e) {
    //     logger::error("(ZHIGUO)error: {}", e.what());
    //     return 1;
    // }

    return 0;
}