#pragma once

#include <vector>
#include <unordered_map>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

#include "ObjParser.h"

constexpr auto GPU_BUFFER_NONE_VALUE = 0x0;

struct PaddedVertex {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 norm;
	alignas(16) glm::vec2 tex;

	PaddedVertex() {}

	PaddedVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texture)
		: pos(position), norm(normal), tex(texture)
	{
	}

	static MeshObject<PaddedVertex> Convert(ObjParser::Mesh mesh) {
		MeshObject<PaddedVertex> res = {};

		for (auto vert : mesh.vertexArray)
			res.vertexArray.push_back({vert.position, vert.normal, vert.texcoord});

		res.indexArray = mesh.indexArray;
		return res;
	}

	bool operator == (const PaddedVertex& v)
	{
		bool res = true;

		res &= this->pos == v.pos;

		res &= this->norm == v.norm;

		res &= this->tex == v.tex;

		return res;
	}
};

enum GLBufferTargets {
	NONE = GPU_BUFFER_NONE_VALUE,
	VERTEX_BUFFER = GL_ARRAY_BUFFER,
	INDEX_BUFFER = GL_ELEMENT_ARRAY_BUFFER,
	ATOMIC_COUNTER = GL_ATOMIC_COUNTER_BUFFER,
	COPY_READ = GL_COPY_READ_BUFFER,
	COPY_WRITE = GL_COPY_WRITE_BUFFER,
	DISPATCH_INDIRECT = GL_DISPATCH_INDIRECT_BUFFER,
	DRAW_INDIRECT = GL_DRAW_INDIRECT_BUFFER,

	// ---------------- UNUSED ----------------

	PIXEL_READ = GL_PIXEL_PACK_BUFFER,
	TEXTURE_READ = GL_PIXEL_UNPACK_BUFFER,
	BUFFER_TEXTURE = GL_TEXTURE_BUFFER,

	// ----------------------------------------

	QUERY_RESULT = GL_QUERY_BUFFER,
	SHADER_STORAGE = GL_SHADER_STORAGE_BUFFER,
	TRANSFORM_FEEDBACK = GL_TRANSFORM_FEEDBACK_BUFFER,
	UNIFORM_BUFFER = GL_UNIFORM_BUFFER
};

enum GLMappedBufferAccess {
	READ_ONLY = GL_READ_ONLY,
	WRITE_ONLY = GL_WRITE_ONLY,
	READ_WRITE = GL_READ_WRITE
};

enum GLBufferUsage {
	STREAM_DRAW = GL_STREAM_DRAW,
	STREAM_READ = GL_STREAM_READ,
	STREAM_COPY = GL_STREAM_COPY,
	STATIC_DRAW = GL_STATIC_DRAW,
	STATIC_READ = GL_STATIC_READ,
	STATIC_COPY = GL_STATIC_COPY,
	DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
	DYNAMIC_READ = GL_DYNAMIC_READ,
	DYNAMIC_COPY = GL_DYNAMIC_COPY
};

// For more information: https://wikis.khronos.org/opengl/Buffer_Object#Immutable_Storage
enum GLImmutableBufferBitfieldFlags{
	STATIC_ONLY_BIT					= 0x0,
	ALLOW_MAP_READ_BIT			= GL_MAP_READ_BIT,
	ALLOW_MAP_WRITE_BIT			= GL_MAP_WRITE_BIT,
	ALLOW_MAP_PERSISTENT_BIT	= GL_MAP_PERSISTENT_BIT,
	ALLOW_MAP_COHERENT_BIT		= GL_MAP_COHERENT_BIT,
	DYNAMIC_STORAGE_BIT		= GL_DYNAMIC_STORAGE_BIT,
	CLIENT_STORAGE_BIT		= GL_CLIENT_STORAGE_BIT,
};

// For more information: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml
enum GLMapRangeBitfieldFlags {
	STATIC_BIT					= 0x0,
	MAP_READ_BIT				= GL_MAP_READ_BIT,
	MAP_WRITE_BIT				= GL_MAP_WRITE_BIT,
	MAP_INVALIDATE_RANGE_BIT	= GL_MAP_INVALIDATE_RANGE_BIT,
	MAP_INVALIDATE_BUFFER_BIT	= GL_MAP_INVALIDATE_BUFFER_BIT,
	MAP_FLUSH_EXPLICIT_BIT		= GL_MAP_FLUSH_EXPLICIT_BIT,
	MAP_UNSYNCHRONIZED_BIT		= GL_MAP_UNSYNCHRONIZED_BIT,
	MAP_PERSISTENT_BIT			= GL_MAP_PERSISTENT_BIT,
	MAP_COHERENT_BIT			= GL_MAP_COHERENT_BIT,
};

template <typename T>
class GLBufferBase
{
protected:

	GLuint m_bufferID = 0;
	size_t m_count = 0;
	GLBufferTargets m_lastTarget = NONE;

public:

	inline void Clean() { glDeleteBuffers(1, &m_bufferID); }

	inline void Bind(GLBufferTargets target)
	{
		glBindBuffer(target, m_bufferID);
		m_lastTarget = target;
	}
	inline void Bind(GLBufferTargets target, GLuint index)
	{
		glBindBufferBase(target, index, m_bufferID);
		m_lastTarget = target;
	}

	inline void UnBind() { UnBind(m_lastTarget); }
	inline void UnBind(GLBufferTargets target)
	{
		glBindBuffer(target, 0);
		m_lastTarget = NONE;
	}

	inline T* MapBufferRange(
		size_t offsetCount, 
		size_t lengthCount,
		GLMapRangeBitfieldFlags usage
	) 
	{ 
		return (T*)glMapNamedBufferRange(
			m_bufferID, 
			offsetCount * GetElementSize(),
			lengthCount * GetElementSize(),
			usage
		); 
	}
	inline T* MapBuffer(GLMappedBufferAccess usage) { return (T*)glMapNamedBuffer(m_bufferID, usage); }
	inline void UnmapBuffer() { glUnmapNamedBuffer(m_bufferID); }

	inline const GLuint GetBufferID() { return m_bufferID; }

	inline const size_t GetElementSize() { return sizeof(T); }
	inline const size_t GetCount() { return m_count; }
	inline const size_t GetSize() { return GetElementSize() * GetCount(); }
};

template<typename T>
class GLBuffer : public GLBufferBase<T>
{
private:

public:
	// Initialize mutable buffer
	inline void Init(T* data, size_t count, GLBufferUsage usage)
	{
		assert(this->m_bufferID == 0);

		this->m_count = count;
		glCreateBuffers(1, &this->m_bufferID);
		glNamedBufferData(this->m_bufferID, this->GetSize(), data, usage);
	}
};

template<typename T>
class GLImmutableBuffer : public GLBufferBase<T>
{
private:

public:
	// Initialize immutable buffer
	inline void Init(T* data, size_t count, GLImmutableBufferBitfieldFlags flags)
	{
		assert(this->m_bufferID == 0);

		this->m_count = count;
		glCreateBuffers(1, &this->m_bufferID);
		glNamedBufferStorage(this->m_bufferID, this->GetSize(), data, flags);
	}
};

template<typename VertexT>
struct MeshShaderObject {

	GLImmutableBuffer<VertexT> vertexBuffer = {};
	GLImmutableBuffer<GLuint> indexBuffer = {};

	void Bind() 
	{
		vertexBuffer.Bind(SHADER_STORAGE, 1);
		indexBuffer.Bind(SHADER_STORAGE, 2);
	}

	void UnBind()
	{
		vertexBuffer.UnBind();
		indexBuffer.UnBind();
	}

	void Clean()
	{
		vertexBuffer.Clean();
		indexBuffer.Clean();
	}
};

template<typename VertexT>
MeshShaderObject<VertexT> CreateMeshForMeshShader(
	MeshObject<VertexT> mesh
) 
{
	return CreateMeshForMeshShader(mesh.vertexArray, mesh.indexArray);
}

template<typename VertexT>
MeshShaderObject<VertexT> CreateMeshForMeshShader(
	std::vector<VertexT> vertices,
	std::vector<GLuint> indices
)
{
	MeshShaderObject<VertexT> mesh;

	mesh.vertexBuffer.Init(vertices.data(), vertices.size(), STATIC_ONLY_BIT);
	mesh.indexBuffer.Init(indices.data(), indices.size(), STATIC_ONLY_BIT);

	return mesh;
}

#define MAX_VERTEX_PER_MESHLET 64
#define MAX_INDEX_PER_MESHLET 378

template<typename VertexT>
struct Meshlet {
	VertexT vertices[MAX_VERTEX_PER_MESHLET];
	GLuint indices[MAX_INDEX_PER_MESHLET];
	GLuint vertexCount;
	GLuint indexCount;
};

template<typename VertexT>
std::vector<Meshlet<VertexT>> GenerateMeshlets(
	MeshObject<VertexT> mesh
)
{
	return GenerateMeshlets(mesh.vertexArray, mesh.indexArray);
}

template<typename T>
uint32_t VectorHas(std::vector<T> vec, T element) {
	for (int i = 0; i < vec.size(); ++i)
	{
		if (vec[i] == element) return i;
	}
	
	return UINT32_MAX;
}

template<typename VertexT>
std::vector<Meshlet<VertexT>> GenerateMeshlets(
	std::vector<VertexT> vertices,
	std::vector<GLuint> indices
)
{
	size_t meshletId = 0;
	std::vector<Meshlet<VertexT>> res = { {} };

	// Global - meshlet local index mapping
	std::unordered_map<GLuint, GLuint> vertexMap = {};

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		GLuint tri[3] = {
			indices[i],
			indices[i+1],
			indices[i+2]
		};

		// New vertex check
		GLuint newVerticesNeeded = 0;
		for (int j = 0; j < 3; ++j)
		{
			if (vertexMap.find(tri[j]) == vertexMap.end()) newVerticesNeeded++;
		}

		// Check if we have hit the meshlet size limit
		if (res[meshletId].vertexCount + newVerticesNeeded > MAX_VERTEX_PER_MESHLET ||
			res[meshletId].indexCount + 3 > MAX_INDEX_PER_MESHLET)
		{
			vertexMap.clear();
			res.push_back({});
			meshletId++;
		}

		for (int j = 0; j < 3; ++j)
		{
			// Check if vertex is not inside meshlet
			if (vertexMap.find(tri[j]) == vertexMap.end())
			{
				GLuint newIndx = res[meshletId].vertexCount;
				vertexMap[tri[j]] = newIndx;

				res[meshletId].vertices[newIndx] = vertices[tri[j]];
				res[meshletId].vertexCount++;
			}

			res[meshletId].indices[res[meshletId].indexCount] = vertexMap[tri[j]];
			res[meshletId].indexCount++;
		}
	}

	return res;
}
