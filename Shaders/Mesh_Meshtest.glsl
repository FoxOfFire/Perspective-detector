#version 450
#extension GL_NV_mesh_shader : require

// Set the number of threads per workgroup (always one-dimensional).
layout(local_size_x=3) in; 

// maximum allocation size for each meshlet
layout(max_vertices=3, max_primitives=1) out;

// the primitive type (points,lines or triangles)
layout(triangles) out;

out PerVertexData
{
  vec4 color;
} v_out[];   


uniform float scale = 0.5;

const vec3 vertices[3] = {
	vec3( 1, -1,  0),
	vec3( 0,  1,  0),
	vec3(-1, -1,  0)
};
const vec3 colors[3] = {
	vec3(1.0,0.0,0.0), 
	vec3(0.0,1.0,0.0), 
	vec3(0.0,0.0,1.0)
};

void main()
{
	// Get thread id
	uint thread_id = gl_LocalInvocationID.x;

	// Set position of vertex, indice and color
	// corresponding to thread_id
	gl_MeshVerticesNV[thread_id].gl_Position = vec4(vertices[thread_id], 1.0);
	gl_PrimitiveIndicesNV[thread_id] = thread_id;
	v_out[thread_id].color = vec4(colors[thread_id], 1.0);

	// We are drawing out only one triangle for now
	gl_PrimitiveCountNV = 1;
}