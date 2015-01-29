#include "MyView.hpp"



// STL headers.
#include <cassert>
#include <iostream>
#include <utility>



// Engine headers.
#include <glm/gtc/matrix_transform.hpp>
#include <SceneModel/SceneModel.hpp>
#include <tygra/FileHelper.hpp>
#include <Vertex.hpp>



#pragma region Constructors

MyView::MyView (MyView&& move)
{
    *this = std::move (move);
}


MyView& MyView::operator= (MyView&& move)
{
    if (this != &move)
    {
        m_program       = std::move (move.m_program);
        m_aspectRatio   = std::move (move.m_aspectRatio);

        m_scene         = std::move (move.m_scene);
        m_meshes        = std::move (move.m_meshes);

        m_sceneVAO      = std::move (move.m_sceneVAO);
        m_vertexVBO     = std::move (move.m_vertexVBO);
        m_elementVBO    = std::move (move.m_elementVBO);

        m_instancePool  = std::move (move.m_instancePool);
        m_poolSize      = std::move (move.m_poolSize);

        m_hexTexture    = std::move (move.m_hexTexture);
    }

    return *this;
}

#pragma endregion


#pragma region Getters and setters

void MyView::setScene(std::shared_ptr<const SceneModel::Context> scene)
{
    m_scene = scene;
}

#pragma endregion


#pragma region Window functions

void MyView::windowViewWillStart(std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);
    
    // Ensure the program gets built.
    buildProgram();

    // Retrieve the Sponza data ready for rendering.
    buildMeshData();
    
    // Finally load the hex texture.
    generateTexture2D (m_hexTexture, "hex.png");    
}


void MyView::windowViewDidReset(std::shared_ptr<tygra::Window> window, int width, int height)
{
    // Reset the viewport and calculate the aspect ratio.
    glViewport (0, 0, width, height);
    m_aspectRatio = width / static_cast<float> (height);
}


void MyView::windowViewDidStop(std::shared_ptr<tygra::Window> window)
{
    // Delete the program.
    glDeleteProgram (m_program);
    
    // Delete all VAOs and VBOs.
    glDeleteVertexArrays (1, &m_sceneVAO);
    glDeleteBuffers (1, &m_vertexVBO);
    glDeleteBuffers (1, &m_elementVBO);
    glDeleteBuffers (1, &m_instancePool);

    // Delete all textures.
    glDeleteTextures (1, &m_hexTexture);
}


void MyView::windowViewRender(std::shared_ptr<tygra::Window> window)
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
                textureID       = glGetUniformLocation (m_program, "textureSampler");

    // Set uniform variables.
    glUniformMatrix4fv (projectionID, 1, GL_FALSE, glm::value_ptr (projection));
    glUniformMatrix4fv (viewID, 1, GL_FALSE, glm::value_ptr (view));
    glUniform1i (textureID, 0);

    // Specify the texture to use.
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, m_hexTexture);

    // Specify the VAO to use.
    glBindVertexArray (m_sceneVAO);

    // Bind the instance pool as the active buffer.
    glBindBuffer (GL_ARRAY_BUFFER, m_instancePool);
    
    // Cache a vector full of model and PVM matrices for the rendering.
    std::vector<glm::mat4> matrices { };
    matrices.resize (m_poolSize);

    // Cache the instance size calculation.
    const auto instanceSize = sizeof (glm::mat4) * 2;

    // Iterate through each mesh using instance rendering to reduce GL calls.
    for (const auto& pair : m_meshes)
    {
        // Obtain the each instance for the current mesh.
        const auto& instances   = m_scene->getInstancesByMeshId (pair.first);
        const auto size         = instances.size();

        // Check if we need to do any rendering at all.
        if (size != 0)
        {
            // Cache access to the current mesh.
            const auto& mesh    = pair.second;

            // Update the instance specific matrices.
            for (unsigned int i = 0; i < size; ++i)
            {
                // Obtain the current instances model transformation.
                const auto& model       = static_cast<glm::mat4> (m_scene->getInstanceById (instances[i]).getTransformationMatrix());
             
                // We have both the model and pvm matrices in the buffer so we need an offset.
                const auto offset       = i * 2;

                matrices[offset]        = model;
                matrices[offset + 1]    = projection * view * model;
            }

            // Only buffer the required data to save time.
            glBufferSubData (GL_ARRAY_BUFFER, 0, instanceSize * size, matrices.data());

            // Finally draw all instances at the same time.
            glDrawElementsInstancedBaseVertex (GL_TRIANGLES, mesh.elementCount, GL_UNSIGNED_INT, (GLvoid*) mesh.elementsOffset, size, mesh.verticesIndex);
        }
    }

    // Unbind all buffers.
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
    
    const auto vertexShader                         = compileShaderFromFile (vertexShaderLocation, ShaderType::Vertex);
    const auto fragmentShader                       = compileShaderFromFile (fragmentShaderLocation, ShaderType::Fragment);
    
    // Attach the shaders to the program we created.
    const std::vector<GLchar*> vertexAttributes     = { "position", "normal", "textureCoord", "model", "pvm" };
    const std::vector<GLchar*> fragmentAttributes   = {  };

    attachShader (m_program, vertexShader, vertexAttributes);
    attachShader (m_program, fragmentShader, fragmentAttributes);

    // Link the program
    linkProgram (m_program);
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
    calculateVBOSize (meshes, vertexSize, elementSize);
    
    allocateVBO (m_vertexVBO, vertexSize, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    allocateVBO (m_elementVBO, elementSize, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    
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
        Mesh newMesh { };
        newMesh.verticesIndex   = vertexIndex;
        newMesh.elementsOffset  = elementOffset;
        newMesh.elementCount    = elements.size();
        
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
    glGenBuffers (1, &m_instancePool);
    allocateInstancePool();

    // Now we can construct the VAO and begin rendering!
    constructVAO();
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

        // We store two matrices per instance.
        m_poolSize = highest * 2;

        // Finally resize the buffer to the correct size.
        glBindBuffer (GL_ARRAY_BUFFER, m_instancePool);
        glBufferData (GL_ARRAY_BUFFER, sizeof (glm::mat4) * m_poolSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer (GL_ARRAY_BUFFER, 0);
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
    glBindBuffer (GL_ARRAY_BUFFER, m_instancePool);

    // We'll combine our matrices into a single VBO so we need the stride to be double.
    createInstancedMatrix4 (modelTransform, sizeof (glm::mat4) * 2);
    createInstancedMatrix4 (pvmTransform,   sizeof (glm::mat4) * 2, sizeof (glm::mat4));

    // Unbind all buffers.
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);
}

#pragma endregion


#pragma region OpenGL creation

GLuint compileShaderFromFile (const std::string& fileLocation, const ShaderType shader)
{
    // Read in the shader into a const char*.
    const auto  shaderString = tygra::stringFromFile (fileLocation);
    auto        shaderCode = shaderString.c_str();
    
    // Attempt to compile the shader.
    GLuint shaderID { };

    switch (shader)
    {
        case ShaderType::Vertex:
            shaderID = glCreateShader (GL_VERTEX_SHADER);
            break;

        case ShaderType::Fragment:
            shaderID = glCreateShader (GL_FRAGMENT_SHADER);
            break;
    }

    glShaderSource (shaderID, 1, static_cast<const GLchar**> (&shaderCode), NULL);
    glCompileShader (shaderID);

    // Check whether compilation was successful.
    GLint compileStatus { 0 };

    glGetShaderiv (shaderID, GL_COMPILE_STATUS, &compileStatus);
    
    if (compileStatus != GL_TRUE)
    {
        // Output error information.
        const unsigned int stringLength = 1024;
        GLchar log[stringLength] = "";

        glGetShaderInfoLog (shaderID, stringLength, NULL, log);
        std::cerr << log << std::endl;
    }

    return shaderID;
}


void attachShader (const GLuint program, const GLuint shader, const std::vector<GLchar*>& attributes)
{
    // Check whether we have a valid shader ID before continuing.
    if (shader != 0)
    {
        glAttachShader (program, shader);

        // Add the given attributes to the shader.
        for (unsigned int i = 0; i < attributes.size(); ++i)
        {
            if (attributes[i] != nullptr)
            {
                glBindAttribLocation (program, i, attributes[i]);
            }
        }

        // Flag the shader for deletion.
        glDeleteShader (shader);
    }    
}


bool linkProgram (const GLuint program)
{
    // Attempt to link the program.
    glLinkProgram (program);

    // Test the status for any errors.
    GLint linkStatus  { 0 };
    glGetProgramiv (program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) 
    {
        // Output error information.
        const unsigned int stringLength = 1024;
        GLchar log[stringLength] = "";

        glGetProgramInfoLog (program, stringLength, NULL, log);
        std::cerr << log << std::endl;

        return false;
    }

    return true;
}


void allocateVBO (GLuint& vbo, const size_t size, const GLenum target, const GLenum usage)
{
    glGenBuffers (1, &vbo);
    glBindBuffer (target, vbo);
    glBufferData (target, size, nullptr, usage);
    glBindBuffer (target, 0);
}


void createInstancedMatrix4 (const int attribLocation, const GLsizei stride, const int extraOffset, const int divisor)
{
    // Pre-condition: A valid attribute location has been given.
    if (attribLocation >= 0)
    {
        // We need to go through each column of the matrices creating attribute pointers.
        const int matrixColumns { 4 };
        for (int i = 0; i < matrixColumns; ++i)
        {
            const int current   { attribLocation + i };

            // Enable each column and set the divisor.
            glEnableVertexAttribArray (current);
            glVertexAttribDivisor (current, divisor);

            // Calculate the offsets for each column.
            const auto offset = TGL_BUFFER_OFFSET (sizeof (glm::vec4) * i + extraOffset);

            // Create the columns attribute pointer.
            glVertexAttribPointer (current,  4, GL_FLOAT, GL_FALSE, stride, offset);
        }
    }
}


void generateTexture2D (GLuint& textureBuffer, const std::string& fileLocation)
{
    // Attempt to load the image.
    tygra::Image image = tygra::imageFromPNG (fileLocation);

    if (image.containsData()) 
    {
        // Start by preparing the texture buffer.
        glGenTextures (1, &textureBuffer);
        glBindTexture (GL_TEXTURE_2D, textureBuffer);

        // Enable standard filters.
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,  GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,      GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,      GL_REPEAT);

        // Enable each different pixel format.
        GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };

        // Load the texture into OpenGL.
        glTexImage2D (  GL_TEXTURE_2D, 0, GL_RGBA,          
                      
                        // Dimensions and border.
                        image.width(), image.height(), 0,   
                      
                        // Format and type.
                        pixel_formats[image.componentsPerPixel()], image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
                      
                        // Data.
                        image.pixels());

        // Generate the mipmaps from the loaded texture and finish.
        glGenerateMipmap (GL_TEXTURE_2D);
        glBindTexture (GL_TEXTURE_2D, 0);
    }
}


void calculateVBOSize (const std::vector<SceneModel::Mesh>& meshes, size_t& vertexSize, size_t& elementSize)
{
    // Create temporary accumlators.
    size_t vertices { 0 }, elements { 0 };  

    // We need to loop through each mesh adding up as we go along.
    for (const auto& mesh : meshes)
    {
        vertices += mesh.getPositionArray().size();
        elements += mesh.getElementArray().size();
    }

    // Calculate the final values.
    vertexSize = vertices * sizeof (Vertex);
    elementSize = elements * sizeof (unsigned int);
}

#pragma endregion