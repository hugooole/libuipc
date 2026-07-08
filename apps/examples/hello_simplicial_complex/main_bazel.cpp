// Bazel entry point for hello_simplicial_complex.
// Writes outputs to a runfiles/cwd directory instead of the CMake AssetDir path.
#include <bazel_app/bazel_env.h>
#include <uipc/uipc.h>

int main(int argc, char** argv)
{
    using namespace uipc;
    using namespace uipc::geometry;

    bazel_env::Env env{argv[0]};
    auto           output_path = env.output_dir("hello_simplicial_complex_output");

    // create a regular tetrahedron
    vector<Vector3>  Vs = {Vector3{0, 1, 0},
                           Vector3{0, 0, 1},
                           Vector3{-std::sqrt(3) / 2, 0, -0.5},
                           Vector3{std::sqrt(3) / 2, 0, -0.5}};
    vector<Vector4i> Ts = {Vector4i{0, 1, 2, 3}};

    SimplicialComplex mesh = tetmesh(Vs, Ts);

    SpreadSheetIO       ssio{output_path};
    SimplicialComplexIO io;

    // 1) dump the original geometry
    ssio.write_json("1_origin", mesh);
    ssio.write_csv("1_origin", mesh);

    // 2) label the surface of the mesh
    label_surface(mesh);
    ssio.write_json("2_labeled", mesh);
    ssio.write_csv("2_labeled", mesh);

    // 3) extract the surface of the mesh without orientation
    auto unoriented_surface = extract_surface(mesh);
    ssio.write_json("3_unoriented_surface", unoriented_surface);
    ssio.write_csv("3_unoriented_surface", unoriented_surface);
    io.write(output_path + "2_unoriented_surface.obj", unoriented_surface);

    // 4) label the triangle orientation
    label_triangle_orient(mesh);
    ssio.write_json("4_oriented", mesh);
    ssio.write_csv("4_oriented", mesh);

    // 5) extract the surface of the mesh with orientation
    auto surface = extract_surface(mesh);
    ssio.write_json("5_oriented_surface", surface);
    ssio.write_csv("5_oriented_surface", surface);
    io.write(output_path + "5_oriented_surface.obj", surface);
}
