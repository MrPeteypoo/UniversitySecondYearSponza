#pragma once

#if !defined    _UTIL_SCENE_MODEL_
#define         _UTIL_SCENE_MODEL_


// STL headers.
#include <vector>


// Forward declarations.
namespace SceneModel { class Material; class Mesh; }
namespace tygra { class Image; }
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


    /// <summary> Iterates through every material in a scene and fills the given vector with image data. </summary>
    /// <param name="images"> The vector to fill with data. This will generate filename-image pairs. </param>
    /// <param name="materials"> A container of materials to iterate through. </param>
    void loadImagesFromScene (std::vector<std::pair<std::string, tygra::Image>>& images, const std::vector<SceneModel::Material>& materials);
}

#endif // _UTIL_SCENE_MODEL_