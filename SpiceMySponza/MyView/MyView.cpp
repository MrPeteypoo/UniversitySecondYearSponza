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
        m_program           = move.m_program;

        m_sceneVAO          = move.m_sceneVAO;
        m_vertexVBO         = move.m_vertexVBO;
        m_elementVBO        = move.m_elementVBO;
        m_uniformUBO        = move.m_uniformUBO;
        m_textureArray      = move.m_textureArray;
        m_materials         = std::move (move.m_materials);
        
        m_instancePoolSize  = move.m_instancePoolSize;
        m_poolTransforms    = move.m_poolTransforms;
        m_poolMaterialIDs   = std::move (move.m_poolMaterialIDs);
        
        m_aspectRatio       = move.m_aspectRatio;

        m_scene             = std::move (move.m_scene);
        m_meshes            = std::move (move.m_meshes);
        m_materials         = std::move (move.m_materials);

        // Reset primitives.
        move.m_program          = 0;

        move.m_sceneVAO         = 0;
        move.m_vertexVBO        = 0;
        move.m_elementVBO       = 0;
        move.m_uniformUBO       = 0;
        move.m_textureArray     = 0;

        move.m_instancePoolSize = 0;
        move.m_poolTransforms    = 0;

        move.m_aspectRatio      = 0.f;
    }

    return *this;
}


MyView::SamplerBuffer::SamplerBuffer (SamplerBuffer&& move)
{
    *this = std::move (move);
}


MyView::SamplerBuffer& MyView::SamplerBuffer::operator= (SamplerBuffer&& move)
{
    if (this != &move)
    {
        vbo = move.vbo;
        tbo = move.tbo;

        // Reset primitives.
        move.vbo = 0;
        move.tbo = 0;
    }

    return *this;
}


#pragma endregion


#pragma region Public interface

void MyView::setScene (std::shared_ptr<const SceneModel::Context> scene)
{
    m_scene = scene;
}


void MyView::rebuildScene()
{
    // Rebuild the entire scene bruv!
    windowViewDidStop (nullptr);
    windowViewWillStart (nullptr);
}

#pragma endregion


#pragma region Scene construction

void MyView::windowViewWillStart (std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);
    
    // Set up OpenGL as required by the application!
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);
    glClearColor (0.f, 0.1f, 0.f, 0.f);
    
    // Attempt to build the program, if it fails the user can reload after correcting any syntax errors.
    if (buildProgram())
    {
        std::cout << "OpenGL application built successfully." << std::endl;
    }

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
    glGenBuffers (1, &m_materials.vbo);
    glGenBuffers (1, &m_poolTransforms);
    glGenBuffers (1, &m_poolMaterialIDs.vbo);
    
    glGenTextures (1, &m_textureArray);
    glGenTextures (1, &m_materials.tbo);
    glGenTextures (1, &m_poolMaterialIDs.tbo);
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
    GLint   vertexIndex      { 0 }; 
    GLint   elementOffset    { 0 };
    
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
        glBufferSubData (GL_ARRAY_BUFFER,           vertexIndex * sizeof (Vertex),  vertices.size() * sizeof (Vertex),          vertices.data());
        glBufferSubData (GL_ELEMENT_ARRAY_BUFFER,   elementOffset,                  elements.size() * sizeof (unsigned int),    elements.data());

        // The vertexIndex needs an actual index value whereas elementOffset needs to be in bytes.
        vertexIndex += vertices.size();
        elementOffset += elements.size() * sizeof (unsigned int);

        // Finally create the pair and add the mesh to the vector.
        m_meshes[i] = { mesh.getId(), std::move (newMesh) };
    }

    // Unbind the buffers.
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}


void MyView::allocateExtraBuffers()
{
    /// Use DYNAMIC for the UBO because we'll only be updating once per frame but using for every instance in the scene.
    /// Use STREAM for the instancing buffers because they will be updated once per mesh and only used for that mesh.

    // We'll need to keep track of the highest number of instances in the scene.
    m_instancePoolSize          = highestInstanceCount();

    // We need to store two matrices per instance and we need to ensure the VBO aligns to a glm::vec4.
    const auto transformSize    = m_instancePoolSize * sizeof (glm::mat4) * 2;
    const auto materialIDSize   = m_instancePoolSize * sizeof (MaterialID);

    // The UBO will contain every uniform variable apart from textures. 
    util::allocateBuffer (m_uniformUBO, sizeof (UniformData), GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);

    // Use
    // The matrices pool stores the model and PVM transformation matrices of each instance, therefore we need two.
    util::allocateBuffer (m_poolTransforms, transformSize, GL_ARRAY_BUFFER, GL_STREAM_DRAW);

    // The material ID pool contains the instance-specific material ID required for correct shading.
    util::allocateBuffer (m_poolMaterialIDs.vbo, materialIDSize, GL_TEXTURE_BUFFER, GL_STREAM_DRAW);
}


void MyView::buildMaterialData()
{
    // Obtain every material in the scene.
    const auto& materials = m_scene->getAllMaterials();

    // Load all of the images in the scen
    std::vector<std::pair<std::string, tygra::Image>> images { };
    util::loadImagesFromScene (images, materials);

    // Iterate through them creating a buffer-ready material for each ID.
    std::vector<Material> bufferMaterials (materials.size());

    for (size_t id = 0; id < materials.size(); ++id)
    {
        // Cache the material.
        const auto& material            = materials[id];

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
        Material bufferMaterial { };
        bufferMaterial.diffuseColour    = material.getDiffuseColour();
        bufferMaterial.textureID        = textureID;
        bufferMaterial.specularColour   = material.getSpecularColour();
        bufferMaterial.shininess        = material.getShininess();

        // Prepare to add it to the GPU and add the ID to the map. We need to remember that a material takes up two columns so the ID must be multiplied by two.
        bufferMaterials[id] = std::move (bufferMaterial);
        m_materialIDs.emplace (material.getId(), MaterialID (id * 2));
    }

    // Load the materials into the GPU and link the buffers together.
    util::fillBuffer (m_materials.vbo, bufferMaterials, GL_TEXTURE_BUFFER, GL_STATIC_DRAW);

    if (!images.empty())
    {
        prepareTextureData (images[0].second.width(), images[0].second.height(), images.size());
    }

    // Just prepare the materials.
    else
    {
        prepareTextureData (1, 1, 1);
    }

    // Finally load the images onto the GPU.
    loadTexturesIntoArray (images);
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
    glBindBuffer (GL_ARRAY_BUFFER, m_poolTransforms);

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
    // Active the material TBO and point it to the material VBO.
    glBindTexture (GL_TEXTURE_BUFFER, m_materials.tbo);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32F, m_materials.vbo);

    // Active the material IDs TBO and point it to the IDs VBO.
    glBindTexture (GL_TEXTURE_BUFFER, m_poolMaterialIDs.tbo);
    glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA32I, m_poolMaterialIDs.vbo);

    // Enable the 2D texture array and prepare its storage. Use 4 mipmap levels.
    glBindTexture (GL_TEXTURE_2D_ARRAY, m_textureArray);
    glTexStorage3D (GL_TEXTURE_2D_ARRAY, 4, GL_RGBA32F, textureWidth, textureHeight, textureCount);

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

    m_meshes.clear();
    m_materialIDs.clear();
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
    glDeleteBuffers (1, &m_materials.vbo);
    glDeleteBuffers (1, &m_poolMaterialIDs.vbo);
    glDeleteBuffers (1, &m_poolTransforms);

    // Delete all textures.
    glDeleteTextures (1, &m_textureArray);
    glDeleteTextures (1, &m_materials.tbo);
    glDeleteTextures (1, &m_poolMaterialIDs.tbo);
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
    /// For the rendering of the scene I have chosen to implement instancing. A traditional approach of rendering would be looping through each instance,
    /// assigning the correct model and PVM transforms, then drawing that one mesh before repeating the process. I don't use that method here, instead
    /// I loop through mesh, obtain the number of instances, load in the data specific to those instances and draw them all at once, letting the shaders
    /// obtain the correct information. I choose this method because although it doesn't help in a simple scene like sponza; scenes with particle systems,
    /// large-scale mesh duplication and such would really benefit from reducing the overhead that bindings, uniform specification and draw calls cost.
    assert (m_scene != nullptr);

    // Specify shader program to use.
    glUseProgram (m_program);

    // Prepare the screen.
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Define matrices.
    const auto& camera      = m_scene->getCamera();
    const auto  projection  = glm::perspective (camera.getVerticalFieldOfViewInDegrees(), m_aspectRatio, camera.getNearPlaneDistance(), camera.getFarPlaneDistance()),
                view        = glm::lookAt (camera.getPosition(), camera.getPosition() + camera.getDirection(), m_scene->getUpDirection());
    
    // Set the uniforms.
    setUniforms (&projection, &view);
    
    // Specify the VAO to use.
    glBindVertexArray (m_sceneVAO);

    // Specify the buffers to use.
    glBindBuffer (GL_ARRAY_BUFFER, m_poolTransforms);
    glBindBuffer (GL_TEXTURE_BUFFER, m_poolMaterialIDs.vbo);

    // Specify the textures to use.
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D_ARRAY, m_textureArray);

    glActiveTexture (GL_TEXTURE1);
    glBindTexture (GL_TEXTURE_BUFFER, m_materials.tbo);

    glActiveTexture (GL_TEXTURE2);
    glBindTexture (GL_TEXTURE_BUFFER, m_poolMaterialIDs.tbo);

    // Use vectors for storing instancing data> This requires a material ID, a model transform and a PVM transform.
    static std::vector<MaterialID> materialIDs (m_instancePoolSize);
    static std::vector<glm::mat4> matrices (m_instancePoolSize * 2);

    // Iterate through each mesh using instancing to reduce GL calls.
    for (const auto& pair : m_meshes)
    {
        // Obtain the instances to draw for the current mesh.
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

                // Now deal with the materials.
                materialIDs[i]          = m_materialIDs.at (instance.getMaterialId());
            }

            // Only overwrite the required data to speed up the buffering process. Avoid glMapBuffer because it's ridiculously slow in this case.
            glBufferSubData (GL_ARRAY_BUFFER,   0,  sizeof (glm::mat4) * 2 * size,  matrices.data());
            glBufferSubData (GL_TEXTURE_BUFFER, 0,  sizeof (MaterialID) * size,     materialIDs.data());
            
            // Cache access to the current mesh.
            const auto& mesh = pair.second;

            // Finally draw all instances at the same time.
            glDrawElementsInstancedBaseVertex (GL_TRIANGLES, mesh->elementCount, GL_UNSIGNED_INT, (void*) mesh->elementsOffset, size, mesh->verticesIndex);
        }
    }

    // UNBIND IT ALL CAPTAIN!
    glBindVertexArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_TEXTURE_BUFFER, 0);

    glActiveTexture (GL_TEXTURE1);
    glBindTexture (GL_TEXTURE_BUFFER, 0);

    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D_ARRAY, 0);
}


void MyView::setUniforms (const void* const projectionMatrix, const void* const viewMatrix)
{
    //// Fix the stupid lab computers not liking how I don't specify he texture unit and how I like using both on texture unit 0.
    //const auto textures     = glGetUniformLocation (m_program, "textures");
    //const auto materials    = glGetUniformLocation (m_program, "materials");
    //const auto materialIDs  = glGetUniformLocation (m_program, "materialIDs");
    //
    //glUniform1i (textures, m_textureArray);
    //glUniform1i (materials, m_materials.tbo);
    //glUniform1i (materialIDs, m_poolMaterialIDs.tbo);
    //
    ///*glUniform1i (textures, 0);
    //glUniform1i (materials, 1);
    //glUniform1i (materialIDs, 2);*/

    // Create data to fill.
    UniformData data { };

    // Obtain the correct data for the uniforms. We'll need to cast the pointers, this is dirty but it prevents calculating the matrices twice
    // or including GLM in the MyView header.
    if (projectionMatrix && viewMatrix)
    {
        data.setProjectionMatrix (*(glm::mat4*) projectionMatrix);
        data.setViewMatrix (*(glm::mat4*) viewMatrix);
    }

    data.setCameraPosition (m_scene->getCamera().getPosition());
    data.setAmbientColour (m_scene->getAmbientLightIntensity());

    // Obtain the lights in the scene.
    const auto& lights  = m_scene->getAllLights();
    size_t lightCount   = lights.size();

    // Add each light to the data.
    for (size_t i = 0; i < lights.size(); ++i)
    {
        data.setLight (i, lights[i], LightType::Spot);   
    }

    // Enable the wireframe light if necessary.
    if (m_wireframeMode)
    {
        data.setLight (lightCount++, createWireframeLight());
    }

    data.setLightCount (lightCount);

    // Overwrite the current uniform data.
    glBindBuffer (GL_UNIFORM_BUFFER, m_uniformUBO);
    glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (UniformData), &data);

    /// This part here may be confusing. There is only one Uniform Buffer Object in MyView and we use the UniformData class to manage how that 
    /// data is managed by the shaders. Although all of the data is maintained in the class itself, we split it into "scene" and "lighting"
    /// segments. We point the binding blocks to the correct parts of the UBO using the information UniformData gives us.
    ///
    /// I would rather UniformData had a Scene and Lighting class which meant the size and offset calculations were less brittle but that's for
    /// another day.

    // Determine the UBO indices.
    const auto scene = glGetUniformBlockIndex (m_program, "scene");
    const auto lighting = glGetUniformBlockIndex (m_program, "lighting");

    // Bind each part of the UBO to the correct block.
    glUniformBlockBinding (m_program, scene, data.sceneBlock());
    glUniformBlockBinding (m_program, lighting, data.lightingBlock());

    // Use the magic data contained in UniformData to separate the UBO into segments.
    glBindBufferRange (GL_UNIFORM_BUFFER, data.sceneBlock(), m_uniformUBO, data.sceneOffset(), data.sceneSize());
    glBindBufferRange (GL_UNIFORM_BUFFER, data.lightingBlock(), m_uniformUBO, data.lightingOffset(), data.lightingSize());

    // Unbind the buffer.
    glBindBuffer (GL_UNIFORM_BUFFER, 0);
}


Light MyView::createWireframeLight() const
{
    // Create the light.
    Light wireframe { };

    // Fill it with the correct information.
    const auto& camera      = m_scene->getCamera();
    wireframe.position      = camera.getPosition();
    wireframe.direction     = camera.getDirection();

    // Set suitable attenuation values.
    wireframe.aConstant     = 1.0f;
    wireframe.aLinear       = 0.3f;
    wireframe.aQuadratic    = 0.0f;

    // Enable the wireframe and we're done!
    wireframe.emitWireframe = 1;
    wireframe.setType (LightType::Point);

    return wireframe;
}

#pragma endregion