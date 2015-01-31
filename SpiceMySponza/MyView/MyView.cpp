#include "MyView.h"



// STL headers.
#include <cassert>
#include <iostream>
#include <utility>



// Engine headers.
#include <glm/gtc/matrix_transform.hpp>
#include <SceneModel/SceneModel.hpp>
#include <tgl/tgl.h>
#include <tygra/FileHelper.hpp>



// Personal headers.
#include <Misc/Vertex.h>
#include <MyView/Material.h>
#include <MyView/Mesh.h>
#include <MyView/UniformData.h>
#include <Utility/OpenGL.h>
#include <Utility/SceneModel.h>



#pragma region Constructors and destructor

MyView::MyView (MyView&& move)
{
    *this = std::move (move);
}


MyView::~MyView()
{
    // Never leave a byte of data behind!
    windowViewDidStop ({ nullptr });
}


MyView& MyView::operator= (MyView&& move)
{
    if (this != &move)
    {
        m_program       = std::move (move.m_program);

        m_sceneVAO      = std::move (move.m_sceneVAO);
        m_vertexVBO     = std::move (move.m_vertexVBO);
        m_elementVBO    = std::move (move.m_elementVBO);
        m_uniformUBO    = std::move (move.m_uniformUBO);

        m_matricesPool  = std::move (move.m_matricesPool);
        m_materialPool  = std::move (move.m_materialPool);
        m_poolSize      = std::move (move.m_poolSize);

        m_materialTBO   = std::move (move.m_materialTBO);
        
        m_aspectRatio   = std::move (move.m_aspectRatio);

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


#pragma region Scene construction

void MyView::windowViewWillStart (std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);
    
    // Ensure the program gets built.
    if (buildProgram())
    {
        // Generate the buffers.
        generateOpenGLObjects();

        // Retrieve the Sponza data ready for rendering.
        buildMeshData();

        // Allocate the required run-time memory for instancing.
        allocateExtraBuffers();

        // Ensure we have the required materials.
        buildMaterialData();

        // Now we can construct the VAO so we're reading for rendering.
        constructVAO();
    }
}


bool MyView::buildProgram()
{
    // Create the program to attach shaders to.
    m_program                                       = glCreateProgram();

    // Attempt to compile the shaders.
    const auto vertexShaderLocation                 = "sponza_vs.glsl";
    const auto fragmentShaderLocation               = "sponza_fs.glsl";
    
    const auto vertexShader                         = util::compileShaderFromFile (vertexShaderLocation, GL_VERTEX_SHADER);
    const auto fragmentShader                       = util::compileShaderFromFile (fragmentShaderLocation, GL_FRAGMENT_SHADER);
    
    // Attach the shaders to the program we created.
    const std::vector<GLchar*> vertexAttributes     = { "position", "normal", "textureCoord", "model", "pvm" };
    const std::vector<GLchar*> fragmentAttributes   = {  };

    util::attachShader (m_program, vertexShader, vertexAttributes);
    util::attachShader (m_program, fragmentShader, fragmentAttributes);

    // Link the program
    return util::linkProgram (m_program);
}


void MyView::generateOpenGLObjects()
{
    glGenVertexArrays (1, &m_sceneVAO);

    glGenBuffers (1, &m_vertexVBO);
    glGenBuffers (1, &m_elementVBO);
    glGenBuffers (1, &m_uniformUBO);
    glGenBuffers (1, &m_materialPool);
    glGenBuffers (1, &m_matricesPool);

    glGenTextures (1, &m_materialTBO);
    glGenTextures (1, &m_textureArray);
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
    
    util::allocateBuffer (m_vertexVBO, vertexSize, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    util::allocateBuffer (m_elementVBO, elementSize, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
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
        util::assembleVertices (vertices, mesh);

        // Fill the vertex buffer objects with data.
        glBufferSubData (GL_ARRAY_BUFFER,           vertexIndex * sizeof (Vertex),  vertices.size() * sizeof (Vertex),                  vertices.data());
        glBufferSubData (GL_ELEMENT_ARRAY_BUFFER,   elementOffset,                  elements.size() * sizeof (SceneModel::InstanceId),  elements.data());

        // The vertexIndex needs an actual index value whereas elementOffset needs to be in bytes.
        vertexIndex += vertices.size();
        elementOffset += elements.size() * sizeof (SceneModel::InstanceId);

        // Finally create the pair and add the mesh to the vector.
        m_meshes[i] = { mesh.getId(), std::move (newMesh) };
    }

    // Unbind the buffers.
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}


void MyView::allocateExtraBuffers()
{
    // We'll need to keep track of the highest number of instances in the scene.
    m_poolSize = highestInstanceCount();

    // The UBO will contain every uniform variable apart from textures.
    util::allocateBuffer (m_uniformUBO, sizeof (UniformData), GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
        
    // The material pool stores the diffuse and specular properties of each instance for the fragment shader.
    util::allocateBuffer (m_materialPool, sizeof (Material) * m_poolSize, GL_TEXTURE_BUFFER, GL_DYNAMIC_DRAW);

    // The matrices pool stores the model and PVM transformation matrices of each instance, therefore we need two.
    util::allocateBuffer (m_matricesPool, sizeof (glm::mat4) * 2 * m_poolSize, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    
}


void MyView::buildMaterialData()
{
    // Obtain every material in the scene.
    const auto& materials = m_scene->getAllMaterials();

    // Load all of the images in the scen
    std::vector<std::pair<std::string, tygra::Image>> images    { };
    util::loadImagesFromScene (images, materials);

    // We need to prepare OpenGL for the texture data. Enforce the dimensions of the first loaded image.
    if (!images.empty())
    {
        prepareTextureData (images[0].second.width(), images[0].second.height(), images.size());
    }

    // Just prepare the material pool.
    else
    {
        prepareTextureData (1, 1, 1);
    }

    // Load the images onto the GPU.
    loadTexturesIntoArray (images);

    // Iterate through them creating a buffer-ready material for each ID.
    for (const auto& material : materials)
    {
        // Check which texture ID to use. If it can't be determined then -1 indicates none.
        const auto& texture             = material.getAmbientMap();
        float       textureID           = -1.f;

        if (!texture.empty())
        {
            // Determine the textureID.
            for (size_t i = 0; i < images.size(); ++i)
            {
                // 0 means equality.
                if (texture == images[i].first)
                {
                    // Cast back to an int otherwise we may have errors.
                    textureID = (float) i;
                    break;
                }
            }
        }

        // Create a buffer-ready material and fill it with correct data.
        Material* bufferMaterial        = new Material();
        bufferMaterial->diffuseColour   = material.getDiffuseColour();
        bufferMaterial->textureID       = textureID;
        bufferMaterial->specularColour  = material.getSpecularColour();
        bufferMaterial->shininess       = material.getShininess();

        // Add it to the map.
        m_materials.emplace (material.getId(), bufferMaterial);
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

    // Initialise the VAO.
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

    // Now we need to create the instanced matrices attribute pointers.
    glBindBuffer (GL_ARRAY_BUFFER, m_matricesPool);

    // We'll combine our matrices into a single VBO so we need the stride to be double.
    util::createInstancedMatrix4 (modelTransform, sizeof (glm::mat4) * 2);
    util::createInstancedMatrix4 (pvmTransform,   sizeof (glm::mat4) * 2, sizeof (glm::mat4));

    // Unbind all buffers.
    glBindVertexArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}


void MyView::prepareTextureData (const GLsizei textureWidth, const GLsizei textureHeight, const GLsizei textureCount)
{
    // Active the material texture buffer and point it to the material pool VBO.
    glBindTexture (GL_TEXTURE_BUFFER, m_materialTBO);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32F, m_materialPool);

    // Enable the 2D texture array and prepare its storage. Use 8 mipmap levels.
    glBindTexture (GL_TEXTURE_2D_ARRAY, m_textureArray);
    glTexStorage3D (GL_TEXTURE_2D_ARRAY, 8, GL_RGBA, textureWidth, textureHeight, textureCount);

    // Enable standard filters.
    glTexParameteri (GL_TEXTURE_2D_ARRAY,   GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D_ARRAY,   GL_TEXTURE_MIN_FILTER,  GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D_ARRAY,   GL_TEXTURE_WRAP_S,      GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D_ARRAY,   GL_TEXTURE_WRAP_T,      GL_REPEAT);

    // Unbind the textures.
    glBindTexture (GL_TEXTURE_BUFFER, 0);
    glBindTexture (GL_TEXTURE_2D_ARRAY, 0);
}


void MyView::loadTexturesIntoArray (const std::vector<std::pair<std::string, tygra::Image>>& images)
{
    glBindTexture (GL_TEXTURE_2D_ARRAY, m_textureArray); 

    for (size_t i = 0; i < images.size(); ++i)
    {
        // Cache the image.
        const auto& image = images[i].second;

        // Only load the image if it contains data.
        if (image.containsData()) 
        {
            // Enable each different pixel format.
            GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };

            glTexSubImage3D (   GL_TEXTURE_2D_ARRAY, 0, 
                
                                // Offsets.
                                0, 0, i,
                            
                                // Dimensions and border.
                                image.width(), image.height(), 1,   
                      
                                // Format and type.
                                pixel_formats[image.componentsPerPixel()], image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
                      
                                // Data.
                                image.pixels());
        }
    }
    
    // Generate the mipmaps from the loaded texture and finish.
    glGenerateMipmap (GL_TEXTURE_2D_ARRAY);
    glBindTexture (GL_TEXTURE_2D_ARRAY, 0);
}


size_t MyView::highestInstanceCount() const
{
    // We'll need a temporary variable to keep track.
    size_t highest  { 0 };
   
    // Iterate through each mesh ID.
    for (const auto& pair : m_meshes)
    {
        const auto current = m_scene->getInstancesByMeshId (pair.first).size();

        if (current > highest)
        {
            highest = current;
        }
    }

    // Return the calculated figure.
    return highest;
}

#pragma endregion


#pragma region Clean up

void MyView::windowViewDidStop (std::shared_ptr<tygra::Window> window)
{    
    // Clean up after ourselves by getting rid of the stored meshes/materials.
    cleanMeshMaterials();

    // Clean up the OpenGL side of things.
    deleteOpenGLObjects();
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


void MyView::deleteOpenGLObjects()
{
    // Delete the program.
    glDeleteProgram (m_program);
    
    // Delete the VAO.
    glDeleteVertexArrays (1, &m_sceneVAO);
    
    // Delete all VBOs.
    glDeleteBuffers (1, &m_vertexVBO);
    glDeleteBuffers (1, &m_elementVBO);
    glDeleteBuffers (1, &m_uniformUBO);
    glDeleteBuffers (1, &m_materialPool);
    glDeleteBuffers (1, &m_matricesPool);

    // Delete all textures.
    glDeleteTextures (1, &m_materialTBO);
    glDeleteTextures (1, &m_textureArray);
}

#pragma endregion


#pragma region Rendering

void MyView::windowViewDidReset (std::shared_ptr<tygra::Window> window, int width, int height)
{
    // Reset the viewport and recalculate the aspect ratio.
    glViewport (0, 0, width, height);
    m_aspectRatio = width / static_cast<float> (height);
}


void MyView::windowViewRender (std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);

    // Prepare the screen.
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor (0.f, 0.1f, 0.f, 0.f);

    // Specify shader program to use.
    glUseProgram (m_program);
    
    // Define matrices.
    const auto& camera          = m_scene->getCamera();
    const auto  projection      = glm::perspective (camera.getVerticalFieldOfViewInDegrees(), m_aspectRatio, camera.getNearPlaneDistance(), camera.getFarPlaneDistance()),
                view            = glm::lookAt (camera.getPosition(), camera.getPosition() + camera.getDirection(), m_scene->getUpDirection());
    
    // I'd rather setUniforms() done everything but then I'd have to pass through glm::mat4 objects, they aren't easy to forward declare!
    UniformData data { };
    data.setProjectionMatrix (projection);
    data.setViewMatrix (view);
    setUniforms (data);
    
    // Specify the VAO to use.
    glBindVertexArray (m_sceneVAO);

    // Specify the buffers to use.
    glBindBuffer (GL_ARRAY_BUFFER, m_matricesPool);
    glBindBuffer (GL_TEXTURE_BUFFER, m_materialPool);

    // Specify the texture buffers to use.
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_BUFFER, m_materialTBO);
    glBindTexture (GL_TEXTURE_2D_ARRAY, m_textureArray);
    
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
                const auto model        = (glm::mat4) instance.getTransformationMatrix();

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

            // Only overwrite the required data to speed up the buffering process. Avoid glMapBuffer because it's ridiculously slow in this case.
            glBufferSubData (GL_ARRAY_BUFFER, 0,    sizeof (glm::mat4) * 2 * size,  matrices.data());
            glBufferSubData (GL_TEXTURE_BUFFER, 0,  sizeof (Material) * size,       materials.data());
            
            // Cache access to the current mesh.
            const auto& mesh = pair.second;

            // Finally draw all instances at the same time.
            glDrawElementsInstancedBaseVertex (GL_TRIANGLES, mesh->elementCount, GL_UNSIGNED_INT, (GLuint*) mesh->elementsOffset, size, mesh->verticesIndex);
        }
    }

    // Unbind all buffers.
    glBindVertexArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_TEXTURE_BUFFER, 0);
    glBindTexture (GL_TEXTURE_BUFFER, 0);
    glBindTexture (GL_TEXTURE_2D_ARRAY, 0);
}


void MyView::setUniforms (UniformData& data)
{
    // Obtain the correct data for the uniforms.
    data.setCameraPosition (m_scene->getCamera().getPosition());
    data.setAmbientColour (m_scene->getAmbientLightIntensity());

    // Obtain the lights in the scene.
    const auto& lights = m_scene->getAllLights();
    data.setLightCount (lights.size());

    // Add each light to the data.
    for (unsigned int i = 0; i < lights.size(); ++i)
    {
        data.setLight (i, lights[i]);   
    }

    // Map the buffer to overwrite it.
    glBindBuffer (GL_UNIFORM_BUFFER, m_uniformUBO);
    glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (UniformData), &data);

    // Connect the buffer to the shaders.
    const auto index = glGetUniformBlockIndex (m_program, "ubo");
    glUniformBlockBinding (m_program, index, 0);
    glBindBufferBase (GL_UNIFORM_BUFFER, 0, m_uniformUBO);

    // Unbind the buffer.
    glBindBuffer (GL_UNIFORM_BUFFER, 0);
}

#pragma endregion