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
        m_program = std::move (move.m_program);
        m_aspectRatio = std::move (move.m_aspectRatio);

        m_scene = std::move (move.m_scene);
        m_meshes = std::move (move.m_meshes);
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
    assert(m_scene != nullptr);
    
    // Ensure the program gets built.
    buildProgram();

    // Begin to construct sponza.
    const auto& builder = SceneModel::GeometryBuilder();
    const auto& meshes  = builder.getAllMeshes();

    // Iterate through each mesh adding them to the map.
    for (const auto& mesh : meshes)
    {
        // Obtain the required vertex information.
        std::vector<Vertex> vertices { };
        assembleVertices (vertices, mesh);
        
        // Initialise a new mesh.
        Mesh newMesh            = { };
        
        // Obtain the elements.
        const auto& elements    = mesh.getElementArray();
        newMesh.elementCount    = elements.size();

        // Fill the vertex buffer objects with data.
        fillVBO (newMesh.vboVertices, vertices, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
        fillVBO (newMesh.vboElements, elements, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

        // Fill the vertex array object for rendering.
        glGenVertexArrays (1, &newMesh.vao);
        glBindVertexArray (newMesh.vao);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, newMesh.vboElements);

        glBindBuffer (GL_ARRAY_BUFFER, newMesh.vboVertices);        

        glEnableVertexAttribArray (0);
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), TGL_BUFFER_OFFSET (0));

        glEnableVertexAttribArray (1);
        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), TGL_BUFFER_OFFSET (12));

        glEnableVertexAttribArray (2);
        glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), TGL_BUFFER_OFFSET (24));
        
        glBindBuffer (GL_ARRAY_BUFFER, 0);
        glBindVertexArray (0);

        // Finally add the mesh to the map.
        m_meshes.emplace (mesh.getId(), std::move (newMesh));
    }    
    
    // Finally load the hex texture.
    tygra::Image texture_image = tygra::imageFromPNG ("hex.png");
    if (texture_image.containsData()) 
    {
        glGenTextures(1, &m_hexTexture);
        glBindTexture(GL_TEXTURE_2D, m_hexTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                       GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     texture_image.width(),
                     texture_image.height(),
                     0,
                     pixel_formats[texture_image.componentsPerPixel()],
                     texture_image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE
                                                            : GL_UNSIGNED_SHORT,
                     texture_image.pixels());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}


void MyView::windowViewDidReset(std::shared_ptr<tygra::Window> window, int width, int height)
{
    glViewport(0, 0, width, height);
    m_aspectRatio = width / static_cast<float> (height);
}


void MyView::windowViewDidStop(std::shared_ptr<tygra::Window> window)
{
    glDeleteProgram (m_program);
    
    for (const auto& pair : m_meshes)
    {
        const auto& mesh = pair.second;

        glDeleteBuffers (1, &mesh.vboVertices);
        glDeleteBuffers (1, &mesh.vboElements);
        glDeleteVertexArrays (1, &mesh.vao);
    }

    glDeleteTextures (1, &m_hexTexture);
}


void MyView::windowViewRender(std::shared_ptr<tygra::Window> window)
{
    assert (m_scene != nullptr);

    // Prepare the screen.
    glEnable (GL_DEPTH_TEST);
    glClearColor (0.f, 0.1f, 0.f, 0.f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Define matrices.
    const auto  camera       = m_scene->getCamera();
    const auto  projection   = glm::perspective (camera.getVerticalFieldOfViewInDegrees(), m_aspectRatio, camera.getNearPlaneDistance(), camera.getFarPlaneDistance()),
                view         = glm::lookAt (camera.getPosition(), camera.getPosition() + camera.getDirection(), glm::vec3 (0, 1, 0));

    // Specify shader program to use.
    glUseProgram (m_program);

    // Get uniform locations.
    const auto  pvmID       = glGetUniformLocation (m_program, "projectionViewModel"),
                modelID     = glGetUniformLocation (m_program, "modelTransform"),
                textureID   = glGetUniformLocation (m_program, "textureSampler");

    // Prepare to render each object in the scene.
    const auto& instances   = m_scene->getAllInstances();
    
    for (const auto& instance : instances)
    {        
        // Finish creating the required matricies.
        const auto model    = glm::mat4 (instance.getTransformationMatrix());
        const auto pvm      = projection * view * model;

        // Specify uniform values.
        glUniformMatrix4fv (pvmID, 1, GL_FALSE, glm::value_ptr (pvm));
        glUniformMatrix4fv (modelID, 1, GL_FALSE, glm::value_ptr (model));

        // Obtain the correct mesh.
        const auto& mesh    = m_meshes.at (instance.getMeshId());

        // Specify VAO to use.
        glBindVertexArray (mesh.vao);

        // Specify the texture to use.
        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, m_hexTexture);

        // Set never changing uniforms.
        glUniform1i (textureID, 0);

        // Draw.
        glDrawElements (GL_TRIANGLES, mesh.elementCount, GL_UNSIGNED_INT, 0);
    }
}

#pragma endregion


#pragma region Utility functions

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
    const std::vector<GLchar*> vertexAttributes     = { "vertexPosition", "vertexNormal", "texturePoint" };
    const std::vector<GLchar*> fragmentAttributes   = {  };

    attachShader (m_program, vertexShader, vertexAttributes);
    attachShader (m_program, fragmentShader, fragmentAttributes);

    // Link the program
    linkProgram (m_program);
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

#pragma endregion


#pragma region OpenGL creation

GLuint compileShaderFromFile (const std::string& fileLocation, const ShaderType shader)
{
    // Read in the shader into a const char*.
    const std::string shaderString = tygra::stringFromFile (fileLocation);
    const char* shaderCode = shaderString.c_str();
    
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

#pragma endregion