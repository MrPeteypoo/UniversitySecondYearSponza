#version 330


/// The uniform buffer scene specific information.
layout (std140) uniform scene
{
    mat4    projection;         //!< The projection transform which establishes the perspective of the vertex.
    mat4    view;               //!< The view transform representing where the camera is looking.

    vec3    cameraPosition;     //!< Contains the position of the camera in world space.
    vec3    ambience;           //!< The ambient lighting in the scene.
};


layout (location = 0)   in      vec3    position;       //!< The local position of the current vertex.
layout (location = 1)   in      vec3    normal;         //!< The local normal vector of the current vertex.
layout (location = 2)   in      vec2    textureCoord;   //!< The texture co-ordinates for the vertex, used for mapping a texture to the object.

layout (location = 3)   in      mat4    model;          //!< The model transform representing the position and rotation of the object in world space.
layout (location = 7)   in      mat4    pvm;            //!< A combined matrix of the project, view and model transforms.


                        out     vec3    worldPosition;  //!< The world position to be interpolated for the fragment shader.
                        out     vec3    worldNormal;    //!< The world normal to be interpolated for the fragment shader.
                        out     vec3    baryPoint;      //!< The barycentric co-ordinate to be interpolated for the fragment shader.
                        out     vec2    texturePoint;   //!< The texture co-ordinate for the fragment to use for texture mapping.
flat                    out     int     instanceID;     //!< Allows the fragment shader to fetch the correct colour data.


/// Determines the desired barycentric co-ordinate of the vertex based on its vertex ID.
/// Returns the barycentric co-ordinate.
vec3 barycentric();


void main()
{
    // Deal with the outputs first.
    worldPosition = mat4x3 (model) * vec4 (position, 1.0);
    worldNormal = mat3 (model) * normal;

    baryPoint = barycentric();
    texturePoint = textureCoord;

    instanceID = gl_InstanceID;

    // Place the vertex in the correct position on-screen.
    gl_Position = pvm * vec4 (position, 1.0);
}


vec3 barycentric()
{
    /// For the implementation of the wireframe toggle I chose to make it completely shader based using barycentric co-ordinates. Using the 
    /// ID assigned to each vertex by OpenGL I can pass a barycentric co-ordinate to the fragment shader which represents each point on the triangle. 
    /// This is interpolated by the GPU and can be used to determine if the current fragment represents part of the wireframe. We can then decide
    /// whether to colour the fragment using Phong or colour it white to visually represent the wireframe.
    /// 
    /// I chose this route for multiple reasons; firstly I use the vertex ID because it has the same effect as assigning each vertex an extra
    /// attribute for little run-time cost. This reduces memory consumption and means the GPU can process more vertices due to more bandwidth being 
    /// available. Secondly, it is very easy to implement and although it can miss some lines, the end result is a very convincing wireframe around objects.
    /// Thirdly, it allows me to have wireframe functionality without performing a second pass through the rendering process which performing the task
    /// on the OpenGL side would require. Finally if I had access to the geometry shader I would manage the barycentric co-ordinates there so that I'm not
    /// doing a division for every vertex. Unfortunately I don't so I have to use the Vertex ID instead.
    ///
    /// Some basic testing shows this method rendering speed by ~3% which is fairly negligible.

    
    // Unfortunately the lab GPUs require the co-ordinates be weighted in a special way for the effect to be correct.
    // My AMD card at home works fine with (1, 0, 0), (0, 1, 0) and (0, 0, 1) and this way.
    const float weight = 100;
    switch (gl_VertexID % 3)
    {
        case 0:
            return vec3 (weight, 1, 0);
        case 1:
            return vec3 (0, weight, 1);
        case 2:
            return vec3 (1, 0, weight);
    }
}