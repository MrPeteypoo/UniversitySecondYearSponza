#pragma once

#if !defined    _UTIL_SCENE_MODEL_
#define         _UTIL_SCENE_MODEL_


// STL headers.
#include <vector>


// Forward declarations.
namespace SceneModel { class Context; class Mesh; }
struct Vertex;


namespace util
{
    /// <summary> Fills a given vector with vertex information which is obtained from the given mesh. </summary>
    /// <param name="vertices"> An array to be filled with Vertex information. </param>
    /// <param name="mesh"> The mesh to retrieve Vertex data from. </param>
    void assembleVertices (std::vector<Vertex>& vertices, const SceneModel::Mesh& mesh);
    

    /// <summary> Iterates through each SceneMode::Mesh in meshes calculating the total buffer size required for a vertex VBO and element VBO. </summary>
    /// <param name="meshes"> A container of all meshes which will exist in a VBO. </param>
    /// <param name="vertexSize"> The calculated size that a vertex array buffer needs to be. </param>
    /// <param name="elementSize"> The calculated size that an element array buffer needs to be. </param>
    void calculateVBOSize (const std::vector<SceneModel::Mesh>& meshes, size_t& vertexSize, size_t& elementSize);
}

#endif // _UTIL_SCENE_MODEL_