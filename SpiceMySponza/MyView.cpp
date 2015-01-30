#include "MyView.hpp"



// STL headers.
#include <cassert>
#include <iostream>
#include <utility>



// Engine headers.
#include <glm/gtc/matrix_transform.hpp>
#include <SceneModel/SceneModel.hpp>
#include <tgl/tgl.h>
#include <Material.hpp>
#include <Mesh.hpp>
#include <OpenGL.hpp>
#include <Vertex.hpp>



#pragma region Constructors and destructor

MyView::MyView (MyView&& move)
{
    *this = std::move (move);
}


MyView::~MyView()
{
    cleanMeshMaterials();
}


MyView& MyView::operator= (MyView&& move)
{
    if (this != &move)
    {
        m_program       = std::move (move.m_program);
        m_aspectRatio   = std::move (move.m_aspectRatio);

        m_sceneVAO      = std::move (move.m_sceneVAO);
        m_vertexVBO     = std::move (move.m_vertexVBO);
        m_elementVBO    = std::move (move.m_elementVBO);

        m_matricesPool  = std::move (move.m_matricesPool);
        m_materialPool  = std::move (move.m_materialPool);
        m_poolSize      = std::move (move.m_poolSize);

        m_materialTBO   = std::move (move.m_materialTBO);
        m_hexTexture    = std::move (move.m_hexTexture);

        m_scene         = std::move (move.m_scene);
        m_meshes        = std::move (move.m_meshes);
        m_materials     = std::move (move.m_materials);
    }

    return *this;
}

#pragma endregion


#pragma region Getters and setters

void MyView::setScene (std::shared_ptr<const SceneModel::Context> scene)
{
    m_scene = scene;
}

#pragma endregion


#pragma region Window functions

void MyView::windowViewWillStart (std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);
    
    // Ensure the program gets built.
    buildProgram();

    // Retrieve the Sponza data ready for rendering.
    buildMeshData();

    // Ensure we have the required materials.
    buildMaterialData();
    
    // Finally load the textures.
    util::generateTexture2D (m_hexTexture, "hex.png");
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_BUFFER, m_materialTBO);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32F, m_materialPool);
}


void MyView::windowViewDidReset (std::shared_ptr<tygra::Window> window, int width, int height)
{
    // Reset the viewport and calculate the aspect ratio.
    glViewport (0, 0, width, height);
    m_aspectRatio = width / static_cast<float> (height);
}


void MyView::windowViewDidStop (std::shared_ptr<tygra::Window> window)
{
    // Delete the program.
    glDeleteProgram (m_program);
    cleanMeshMaterials();
    
    // Delete all VAOs and VBOs.
    glDeleteVertexArrays (1, &m_sceneVAO);
    glDeleteBuffers (1, &m_vertexVBO);
    glDeleteBuffers (1, &m_elementVBO);
    glDeleteBuffers (1, &m_matricesPool);

    // Delete all textures.
    glDeleteTextures (1, &m_materialTBO);
    glDeleteTextures (1, &m_hexTexture);
}


void MyView::windowViewRender (std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);

    // Prepare the screen.
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor (0.f, 0.1f, 0.f, 0.f);
    
    // Define matrices.
    const auto& camera          = m_scene->getCamera();
    const auto  projection      = glm::perspective (camera.getVerticalFieldOfViewInDegrees(), m_aspectRatio, camera.getNearPlaneDistance(), camera.getFarPlaneDistance()),
                view            = glm::lookAt (camera.getPosition(), camera.getPosition() + camera.getDirection(), m_scene->getUpDirection());

    // Specify shader program to use.
    glUseProgram (m_program);

    // Get uniform locations.
    const auto  projectionID    = glGetUniformLocation (m_program, "projection"),
                viewID          = glGetUniformLocation (m_program, "view"),

                textureID       = glGetUniformLocation (m_program, "textureSampler"),
                cameraPosID     = glGetUniformLocation (m_program, "cameraPosition");

    // Set uniform variables.
    glUniformMatrix4fv (projectionID, 1, GL_FALSE, glm::value_ptr (projection));
    glUniformMatrix4fv (viewID, 1, GL_FALSE, glm::value_ptr (view));
    glUniform3fv (cameraPosID, 1, glm::value_ptr (camera.getPosition()));
    glUniform1i (textureID, 0);

    // Specify the VAO to use.
    glBindVertexArray (m_sceneVAO);

    // Specify the textures to use.
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_BUFFER, m_materialTBO);

    glActiveTexture (GL_TEXTURE1);
    glBindTexture (GL_TEXTURE_2D, m_hexTexture);

    // Bind the instance pools as the active buffer.
    glBindBuffer (GL_ARRAY_BUFFER, m_matricesPool);
    glBindBuffer (GL_TEXTURE_BUFFER, m_materialPool);
    
    // Cache a vector full of model and PVM matrices for the rendering.
    std::vector<glm::mat4> matrices { };
    matrices.resize (m_poolSize * 2);

    // Cache a vector full of material colouring data for the rendering.
    std::vector<Material> materials { };
    materials.resize (m_poolSize);

    // Iterate through each mesh using instance rendering to reduce GL calls.
    for (const auto& pair : m_meshes)
    {
        // Obtain the each instance for the current mesh.
        const auto& instances   = m_scene->getInstancesByMeshId (pair.first);
        const auto size         = instances.size();

        // Check if we need to do any rendering at all.
        if (size != 0)
        {
            // Update the instance-specific information.
            for (unsigned int i = 0; i < size; ++i)
            {
                // Cache the current instance.
                const auto& instance    = m_scene->getInstanceById (instances[i]);

                // Obtain the current instances model transformation.
                const auto model        = static_cast<glm::mat4> (instance.getTransformationMatrix());

                // We have both the model and pvm matrices in the buffer so we need an offset.
                const auto offset       = i * 2;
                matrices[offset]        = model;
                matrices[offset + 1]    = projection * view * model;

                // Now deal with the materials. Assuming no errors occur a try-catch block should have almost no overhead.
                const auto materialID   = instance.getMaterialId();
                try
                {
                    // Use .at() to throw an exception instead of creating an access violation error.
                    materials[i] = *m_materials.at (materialID);
                }

                catch (...)
                {
                    // Create a blank material and use that.
                    m_materials[materialID] = new Material();
                    materials[i]            = *m_materials.at (materialID);
                }
            }

            // Only overwrite the required data to speed up the buffering process.
            glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (glm::mat4) * 2 * size, matrices.data());
            glBufferSubData (GL_TEXTURE_BUFFER, 0, sizeof (Material) * size, materials.data());
            
            // Cache access to the current mesh.
            const auto& mesh = pair.second;

            // Finally draw all instances at the same time.
            glDrawElementsInstancedBaseVertex (GL_TRIANGLES, mesh->elementCount, GL_UNSIGNED_INT, (GLuint*) mesh->elementsOffset, size, mesh->verticesIndex);
        }
    }

    // Unbind all buffers.
    glBindBuffer (GL_TEXTURE_BUFFER, 0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);
}

#pragma endregion


#pragma region Scene construction

void MyView::buildProgram()
{
    // Create the program to attach shaders to.
    m_program                                       = glCreateProgram();

    // Attempt to compile the shaders.
    const auto vertexShaderLocation                 = "sponza_vs.glsl";
    const auto fragmentShaderLocation               = "sponza_fs.glsl";
    
    const auto vertexShader                         = util::compileShaderFromFile (vertexShaderLocation, util::ShaderType::Vertex);
    const auto fragmentShader                       = util::compileShaderFromFile (fragmentShaderLocation, util::ShaderType::Fragment);
    
    // Attach the shaders to the program we created.
    const std::vector<GLchar*> vertexAttributes     = { "position", "normal", "textureCoord", "model", "pvm" };
    const std::vector<GLchar*> fragmentAttributes   = {  };

    util::attachShader (m_program, vertexShader, vertexAttributes);
    util::attachShader (m_program, fragmentShader, fragmentAttributes);

    // Link the program
    util::linkProgram (m_program);
}


void MyView::buildMeshData()
{
    // Begin to construct sponza.
    const auto& builder = SceneModel::GeometryBuilder();
    const auto& meshes  = builder.getAllMeshes();

    // Resize our vector to speed up the loading process.
    m_meshes.resize (meshes.size());

    // Start by allocating enough memory in the VBOs to contain the scene.
    size_t vertexSize { 0 }, elementSize { 0 };
    util::calculateVBOSize (meshes, vertexSize, elementSize);
    
    util::allocateVBO (m_vertexVBO, vertexSize, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    util::allocateVBO (m_elementVBO, elementSize, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
    // Bind our VBOs.
    glBindBuffer (GL_ARRAY_BUFFER, m_vertexVBO);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_elementVBO);

    // Iterate through each mesh adding them to the mesh container.
    GLint  vertexIndex      { 0 }; 
    GLuint elementOffset    { 0 };
    
    for (unsigned int i = 0; i < meshes.size(); ++i)
    {
        // Cache the current mesh.
        const auto& mesh        = meshes[i];
        const auto& elements    = mesh.getElementArray();
        
        // Initialise a new mesh.
        Mesh* newMesh { new Mesh() };
        newMesh->verticesIndex   = vertexIndex;
        newMesh->elementsOffset  = elementOffset;
        newMesh->elementCount    = elements.size();
        
        // Obtain the required vertex information.
        std::vector<Vertex> vertices { };
        assembleVertices (vertices, mesh);

        // Fill the vertex buffer objects with data.
        glBufferSubData (GL_ARRAY_BUFFER,           vertexIndex * sizeof (Vertex),  vertices.size() * sizeof (Vertex),                  vertices.data());
        glBufferSubData (GL_ELEMENT_ARRAY_BUFFER,   elementOffset,                  elements.size() * sizeof (SceneModel::InstanceId),  elements.data());

        // The vertexIndex needs an actual index value whereas elementOffset needs to be in bytes.
        vertexIndex += vertices.size();
        elementOffset += elements.size() * sizeof (SceneModel::InstanceId);

        // Finally create the pair and add the mesh to the vector.
        m_meshes[i] = { mesh.getId(), std::move (newMesh) };
    }    

    // Generate the instance pool buffer for the VAO.
    glGenBuffers (1, &m_matricesPool);
    glGenBuffers (1, &m_materialPool);
    allocateInstancePool();

    // Now we can construct the VAO and begin rendering!
    constructVAO();
}


void MyView::buildMaterialData()
{
    // Obtain every material in the scene.
    const auto& materials = m_scene->getAllMaterials();

    // Iterate through them creating a buffer-ready material for each ID.
    for (const auto& material : materials)
    {
        Material* bufferMaterial        = new Material();
        bufferMaterial->diffuseColour   = material.getDiffuseColour();
        bufferMaterial->specularColour  = material.getSpecularColour();
        bufferMaterial->shininess       = material.getShininess();

        // Add it to the map.
        m_materials.emplace (material.getId(), bufferMaterial);
    }
}


void MyView::assembleVertices (std::vector<Vertex>& vertices, const SceneModel::Mesh& mesh)
{
    // Obtain each attribute.
    const auto& positions       = mesh.getPositionArray();
    const auto& normals         = mesh.getNormalArray();
    const auto& texturePoints   = mesh.getTextureCoordinateArray();

    // Check how much data we need to allocate.
    const auto size             = positions.size();
    vertices.resize (size);

    // Fill the actual data.
    for (unsigned int i = 0; i < size; ++i)
    {
        vertices[i] = { positions[i], normals[i], texturePoints[i] };
    }
}


void MyView::allocateInstancePool()
{
    // Pre-condition: We have a valid scene assigned.
    if (m_scene)
    {
        // We'll need to keep track of the highest number of instances in the scene.
        size_t highest = 0;

        for (const auto& pair : m_meshes)
        {
            // Obtain the number of instances for the current mesh.
            const size_t instances = m_scene->getInstancesByMeshId (pair.first).size();

            // Update the highest value if necessary.
            if (instances > highest)
            {
                highest = instances;
            }
        }

        // Update the pool size.
        m_poolSize = highest;

        // Finally resize the buffers to the correct size. Remember we need two matrices.
        glBindBuffer (GL_ARRAY_BUFFER, m_matricesPool);
        glBindBuffer (GL_TEXTURE_BUFFER, m_materialPool);

        glBufferData (GL_ARRAY_BUFFER, sizeof (glm::mat4) * 2 * m_poolSize, nullptr, GL_DYNAMIC_DRAW);
        glBufferData (GL_TEXTURE_BUFFER, sizeof (Material) * m_poolSize, nullptr, GL_DYNAMIC_DRAW);
        
        // Unbind the buffers.
        glBindBuffer (GL_ARRAY_BUFFER, 0);
        glBindBuffer (GL_TEXTURE_BUFFER, 0);
    }
}


void MyView::constructVAO()
{
    // Obtain the attribute pointer locations we'll be using to construct the VAO.
    int position        { glGetAttribLocation (m_program, "position") };
    int normal          { glGetAttribLocation (m_program, "normal") };
    int textureCoord    { glGetAttribLocation (m_program, "textureCoord") };

    int modelTransform  { glGetAttribLocation (m_program, "model") };
    int pvmTransform    { glGetAttribLocation (m_program, "pvm") };

    // Generate the VAO.
    glGenVertexArrays (1, &m_sceneVAO);
    glBindVertexArray (m_sceneVAO);

    // Bind the element buffer to the VAO.
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_elementVBO);

    // Enable each attribute pointer.
    glEnableVertexAttribArray (position);
    glEnableVertexAttribArray (normal);
    glEnableVertexAttribArray (textureCoord);

    // Begin creating the vertex attribute pointer from the interleaved buffer.
    glBindBuffer (GL_ARRAY_BUFFER, m_vertexVBO);

    // Set the properties of each attribute pointer.
    glVertexAttribPointer (position,        3, GL_FLOAT, GL_FALSE, sizeof (Vertex), TGL_BUFFER_OFFSET (0));
    glVertexAttribPointer (normal,          3, GL_FLOAT, GL_FALSE, sizeof (Vertex), TGL_BUFFER_OFFSET (12));
    glVertexAttribPointer (textureCoord,    2, GL_FLOAT, GL_FALSE, sizeof (Vertex), TGL_BUFFER_OFFSET (24));

    // Now we need to create the instanced matrix attribute pointers.
    glBindBuffer (GL_ARRAY_BUFFER, m_matricesPool);

    // We'll combine our matrices into a single VBO so we need the stride to be double.
    util::createInstancedMatrix4 (modelTransform, sizeof (glm::mat4) * 2);
    util::createInstancedMatrix4 (pvmTransform,   sizeof (glm::mat4) * 2, sizeof (glm::mat4));

    // Unbind all buffers.
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);
}


void MyView::cleanMeshMaterials()
{
    // Clean the mesh vector.
    for (auto& pair : m_meshes)
    {
        if (pair.second)
        {
            delete pair.second;
            pair.second = nullptr;
        }
    }

    // Clean the materials map.
    for (auto& pair : m_materials)
    {
        if (pair.second)
        {
            delete pair.second;
            pair.second = nullptr;
        }
    }

    m_meshes.clear();
    m_materials.clear();
}

#pragma endregion