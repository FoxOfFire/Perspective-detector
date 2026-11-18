#include "MyApp.h"

#include "GLMeshShaderUtils.h"
#include "SDL_GLDebugMessageCallback.h"
#include "ObjParser.h"
#include "ProgramBuilder.h"

#include <array>
#include <glm/ext/vector_int2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <cmath>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

CMyApp::CMyApp() : m_cameraManipulator(m_usedCamera)
{

	if(!GLEW_NV_mesh_shader)
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[GL NV] This hardware does not support mesh shaders.");
	else
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] This hardware supports mesh shaders.");

	GLint max_draw_mesh_tasks_count;
	glGetIntegerv(GL_MAX_DRAW_MESH_TASKS_COUNT_NV, &max_draw_mesh_tasks_count);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] GL_MAX_DRAW_MESH_TASKS_COUNT_NV = %i (at least: 65,535)", max_draw_mesh_tasks_count);
	glm::ivec3 max_mesh_work_group_size;
	glGetIntegeri_v(GL_MAX_MESH_WORK_GROUP_SIZE_NV, 0, &max_mesh_work_group_size.x);
	glGetIntegeri_v(GL_MAX_MESH_WORK_GROUP_SIZE_NV, 1, &max_mesh_work_group_size.y);
	glGetIntegeri_v(GL_MAX_MESH_WORK_GROUP_SIZE_NV, 2, &max_mesh_work_group_size.z);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] GL_MAX_MESH_WORK_GROUP_SIZE_NV = %i x %i x %i", max_mesh_work_group_size.x, max_mesh_work_group_size.y, max_mesh_work_group_size.z);
	glm::ivec3 max_task_work_group_size;
	glGetIntegeri_v(GL_MAX_TASK_WORK_GROUP_SIZE_NV, 0, &max_task_work_group_size.x);
	glGetIntegeri_v(GL_MAX_TASK_WORK_GROUP_SIZE_NV, 1, &max_task_work_group_size.y);
	glGetIntegeri_v(GL_MAX_TASK_WORK_GROUP_SIZE_NV, 2, &max_task_work_group_size.z);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] GL_MAX_TASK_WORK_GROUP_SIZE_NV = %i x %i x %i", max_task_work_group_size.x, max_task_work_group_size.y, max_task_work_group_size.z);
	GLint max_mesh_output_vertices;
	glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &max_mesh_output_vertices);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] GL_MAX_MESH_OUTPUT_VERTICES_NV = %i", max_mesh_output_vertices);
	GLint max_mesh_output_primitives;
	glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &max_mesh_output_primitives);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] GL_MAX_MESH_OUTPUT_PRIMITIVES_NV = %i", max_mesh_output_primitives);
	GLint max_task_output;
	glGetIntegerv(GL_MAX_TASK_OUTPUT_COUNT_NV, &max_task_output);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "[GL NV] GL_MAX_TASK_OUTPUT_COUNT_NV = %i", max_task_output);
}

CMyApp::~CMyApp()
{
}

void CMyApp::SetupDebugCallback()
{
	// engedélyezzük és állítsuk be a debug callback függvényt ha debug context-ben vagyunk
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}

void CMyApp::InitShaders()
{
	m_programID = glCreateProgram();
	//AssembleProgram(m_programID, "Shaders/Vert_PosNormTexMPos.vert", "Shaders/Frag_LightingSimpleMPos.frag");
	// Same result
	ProgramBuilder{ m_programID }
		.ShaderStage(GL_VERTEX_SHADER	,"Shaders/Vert_PosNormTexMPos.vert")
		.ShaderStage(GL_FRAGMENT_SHADER	,"Shaders/Frag_LightingSimpleMPos.frag")
		.Link();

	InitAxesShader();
	InitSkyboxShaders();
	InitMeshShaders();
	InitFlipperShaders();
}

void CMyApp::CleanShaders()
{
	glDeleteProgram(m_programID);
	CleanAxesShader();
	CleanSkyboxShaders();
	CleanMeshShaders();
	CleanFlipperShaders();
}

void CMyApp::InitMeshShaders()
{	m_multipleObjProgram = glCreateProgram();
	ProgramBuilder{ m_multipleObjProgram }
		.ShaderStage(GL_TASK_SHADER_NV, "Shaders/MeshShader/Task_DrawMultipleObj.task")
		.ShaderStage(GL_MESH_SHADER_NV, "Shaders/MeshShader/Mesh_DrawMultipleObj.mesh")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/MeshShader/Frag_MeshDraw.frag")
		.Link();
	
}
void CMyApp::InitFlipperShaders(){
	m_flipperProgram = glCreateProgram();
	ProgramBuilder{m_flipperProgram}
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/flipper.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/flipper.frag")
		.Link();
}
void CMyApp::CleanFlipperShaders(){
	glDeleteProgram(m_flipperProgram);
}

void CMyApp::CleanMeshShaders()
{
	glDeleteProgram(m_multipleObjProgram);
}

void CMyApp::InitAxesShader()
{
	m_programAxesID = glCreateProgram();
	ProgramBuilder{ m_programAxesID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_axes.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Col.frag")
		.Link();
}

void CMyApp::CleanAxesShader()
{
	glDeleteProgram(m_programAxesID);
}

void CMyApp::InitSkyboxShaders()
{
	m_programSkyboxID = glCreateProgram();
	ProgramBuilder{ m_programSkyboxID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_skybox.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_skybox.frag")
		.Link();
}

void CMyApp::CleanSkyboxShaders()
{
	glDeleteProgram(m_programSkyboxID);
}

MeshObject<Vertex> initCube()
{
	MeshObject<Vertex> cubeMeshCPU = {};
	cubeMeshCPU.vertexArray =
	{
		// Front
		{ glm::vec3(-1, -1, 0.0), glm::vec3(0, 0, 1), glm::vec2(0, 0) },
		{ glm::vec3(+1, -1, 0.0), glm::vec3(0, 0, 1), glm::vec2(1, 0) },
		{ glm::vec3(-1, +1, 0.0), glm::vec3(0, 0, 1), glm::vec2(0, 1) },
		{ glm::vec3(+1, +1, 0.0), glm::vec3(0, 0, 1), glm::vec2(1, 1) },
	};

	std::vector<GLuint> indices = {0, 1, 2, 1, 3, 2};
	cubeMeshCPU.indexArray = std::move(indices);
	return cubeMeshCPU;
}

struct MySphereParam
{
	// (x, y, z) = ( r*cos(theta)*sin(phi), r*sin(theta)*sin(phi), r*cos(phi) )
	// where r is radius, theta [0,2*pi[, phi [0,pi[
	float radius;
	MySphereParam(float radius) : radius(radius) {}

	glm::vec3 GetPos(float u, float v) const noexcept
	{
		float theta = glm::pi<float>() * 2.0f * u;
		float phi	= glm::pi<float>() * v;

		return radius * glm::vec3(
						cos(theta) * sin(phi),
						cos(phi), 
						sin(theta) * sin(phi));
	}
	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		float theta	= glm::pi<float>() * 2.0f * u;
		float phi	= glm::pi<float>() * v;

		return glm::vec3(
						cos(theta) * sin(phi),
						cos(phi),
						sin(theta) * sin(phi));
	}
	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

void CMyApp::InitGeometry()
{	
	// Cube
	MeshObject<Vertex> cubeMeshCPU = initCube();
	cubeGPU = CreateMeshForMeshShader(PaddedVertex::Convert(cubeMeshCPU));

	//Post Process vao
	glGenVertexArrays(1,&m_frameVao);
	

	// Skybox
	InitSkyboxGeometry();
}

void CMyApp::CleanGeometry()
{
	cubeGPU.Clean();
	glDeleteVertexArrays(1, &m_frameVao);

	CleanSkyboxGeometry();
}

void CMyApp::InitSkyboxGeometry()
{
	MeshObject<glm::vec3> skyboxCPU
	{
		std::vector<glm::vec3>
		{
			// Back side
			glm::vec3(-1, -1, -1),
			glm::vec3( 1, -1, -1),
			glm::vec3( 1,  1, -1),
			glm::vec3(-1,  1, -1),
			// Front side
			glm::vec3(-1, -1,  1),
			glm::vec3( 1, -1,  1),
			glm::vec3( 1,  1,  1),
			glm::vec3(-1,  1,  1),
		}, // 8 corner of the cube

		std::vector<GLuint>
		{
			// Back side
			0, 1, 2,
			2, 3, 0,
			// Front side
			4, 6, 5,
			6, 4, 7,
			// Left side
			0, 3, 4,
			4, 3, 7,
			// Right side
			1, 5, 2,
			5, 6, 2,
			// Bottom side
			1, 0, 4,
			1, 4, 5,
			// Top side
			3, 2, 6,
			3, 6, 7,
		}
	};

	m_Skybox = CreateGLObjectFromMesh(skyboxCPU, { { 0, offsetof( glm::vec3, x), 3, GL_FLOAT } } );
}

void CMyApp::CleanSkyboxGeometry()
{
	CleanOGLObject( m_Skybox );
}

void CMyApp::InitTextures()
{
	glCreateSamplers(1, &m_SamplerID);
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ImageRGBA MossyImage = ImageFromFile( "Assets/mossy.png" );
	glCreateTextures( GL_TEXTURE_2D, 1, &m_MossyTextureID );
	glTextureStorage2D( m_MossyTextureID, NumberOfMIPLevels(MossyImage), GL_RGBA8, MossyImage.width, MossyImage.height );
	glTextureSubImage2D( m_MossyTextureID, 0, 0, 0, MossyImage.width, MossyImage.height, GL_RGBA, GL_UNSIGNED_BYTE, MossyImage.data() );
	glGenerateTextureMipmap( m_MossyTextureID );

	for (int i = 0; i < m_car_img_cnt; i++) {
		std::stringstream ss;
		ss << "Assets/ELTECar" << i+1 << ".png";
		ImageRGBA CarImage = ImageFromFile( ss.str() );
		glCreateTextures( GL_TEXTURE_2D, 1, &m_CarIDs[i] );
		glTextureStorage2D( m_CarIDs[i], NumberOfMIPLevels(CarImage), GL_RGBA8, CarImage.width, CarImage.height );
		glTextureSubImage2D( m_CarIDs[i], 0, 0, 0, CarImage.width, CarImage.height, GL_RGBA, GL_UNSIGNED_BYTE, CarImage.data() );
		glGenerateTextureMipmap( m_CarIDs[i] );

		int w = CarImage.width;
		int h = CarImage.height;

		m_CarAspects[i] = (float)w / (float)h;
		m_CarSizes[i] = glm::ivec2(w,h);

		m_frames[i] = {CMyApp::MakeBuffer(w,h), CMyApp::MakeBuffer(w, h), CMyApp::MakeBuffer(w, h)};
	}

	m_vec_texes = {
		CMyApp::MakeBuffer(m_vec_tex_size, m_vec_tex_size),
		CMyApp::MakeBuffer(m_vec_tex_size, m_vec_tex_size),
		CMyApp::MakeBuffer(m_vec_tex_size, m_vec_tex_size)
	};
	InitSkyboxTextures();
}

void CMyApp::CleanTextures()
{
	glDeleteTextures(1, &m_MossyTextureID);

	CleanSkyboxTextures();

	glDeleteTextures( 1, &m_SamplerID );
}

void CMyApp::InitSkyboxTextures()
{
	// skybox texture
	static const char* skyboxFiles[6] = {
		"Assets/xpos.png",
		"Assets/xneg.png",
		"Assets/ypos.png",
		"Assets/yneg.png",
		"Assets/zpos.png",
		"Assets/zneg.png",
	};

	ImageRGBA images[6];
	for (int i = 0; i < 6; ++i)
	{
		images[i] = ImageFromFile(skyboxFiles[i], false);
	}

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_SkyboxTextureID);
	glTextureStorage2D(m_SkyboxTextureID, 1, GL_RGBA8, images[0].width, images[0].height);

	for (int face = 0; face < 6; ++face)
	{
		glTextureSubImage3D(m_SkyboxTextureID, 0, 0, 0, face, images[face].width, images[face].height, 1, GL_RGBA, GL_UNSIGNED_BYTE, images[face].data());
	}

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void CMyApp::CleanSkyboxTextures()
{
	glDeleteTextures(1, &m_SkyboxTextureID);
}

bool CMyApp::Init()
{
	SetupDebugCallback();

	// Set a bluish clear color
	// glClear() will use this for clearing the color buffer.
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	//
	// OpenGL
	//

	glEnable(GL_CULL_FACE);	 // Enable discarding the back-facing faces.
	glCullFace(GL_BACK);     // GL_BACK: facets facing away from camera, GL_FRONT: facets facing towards the camera
	glEnable(GL_DEPTH_TEST); // Enable depth testing. (for overlapping geometry)

	// Camera
	m_usedCamera.SetView(
		glm::vec3(5, 5, 5),	// From where we look at the scene - eye
		glm::vec3(0, 0, 0),	// Which point of the scene we are looking at - at
		glm::vec3(0, 1, 0)	// Upwards direction - up
	);
	m_cameraManipulator.SetStateFromCamera();

	//
	// Init functions
	//	

	InitGeometry();
	InitTextures();
	InitShaders(); // note: shaders might be recompiled of fixed function pipelines are changed

	{
		std::vector<glm::mat4> cubeWorld ={ glm::scale(glm::vec3(5.0))*glm::scale(glm::vec3(1.0,1.0/m_CarAspects[m_active_car_img],1.0))};

		m_cubeTransforms.Init(cubeWorld.data(), 1, ALLOW_MAP_WRITE_BIT);
	}


	return true;
}

void CMyApp::Clean()
{
	CleanGeometry();
	CleanTextures();
	CleanShaders();

	m_cubeTransforms.Clean();
}

void CMyApp::Update(const SUpdateInfo& updateInfo)
{
	m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;
	m_delta_time = updateInfo.DeltaTimeInSec;

	m_cameraManipulator.Update(updateInfo.DeltaTimeInSec);

	// 
	// Update cube transforms
	// 
	{
		std::vector<glm::mat4> cubeWorld ={ glm::scale(glm::vec3(5.0))*glm::scale(glm::vec3(1.0,1.0/m_CarAspects[m_active_car_img],1.0))};


		auto ptr = m_cubeTransforms.MapBuffer(WRITE_ONLY);
		std::memcpy(ptr, cubeWorld.data(),  sizeof(cubeWorld));
		m_cubeTransforms.UnmapBuffer();
	}
}

void CMyApp::DrawAxes()
{
	// We always want to see it, regardless of whether there is an object in front of it
	glDisable(GL_DEPTH_TEST);
	GLfloat prevLineWidth = 0.0f;
	glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);
	glLineWidth(1.0f);
	glUseProgram(m_programAxesID);

	glm::mat4 axisWorld = (m_lockAxesToCamera) ? glm::translate(m_usedCamera.GetAt()) : glm::identity<glm::mat4>(); // glm::identity<glm::mat4>() == glm::mat4(1.0f)
	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_usedCamera.GetViewProj()));
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(axisWorld));

	glDrawArrays(GL_LINES, 0, 6);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(prevLineWidth);
	glUseProgram(0);
}

void CMyApp::DrawSkybox()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTextureID);

	glUseProgram(m_programSkyboxID);

	glUniform1i(ul("skyboxTexture"), 0);
	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_usedCamera.GetViewProj()));
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(glm::translate(m_usedCamera.GetEye())));

	// Save the last Z-test, namely the relation by which we update the pixel.
	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);

	// Now we use less-then-or-equal, because we push everything to the far clipping plane
	glDepthFunc(GL_LEQUAL);

	glBindVertexArray(m_Skybox.vaoID);
	glDrawElements(GL_TRIANGLES, m_Skybox.count, GL_UNSIGNED_INT, nullptr);

	glDepthFunc(prevDepthFnc);

	glUseProgram(0);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

CMyApp::FrameBuffer CMyApp::MakeBuffer(int w, int h){
	CMyApp::FrameBuffer buffer{};

	glGenTextures(1,&buffer.texture);
	glBindTexture(GL_TEXTURE_2D,buffer.texture);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &buffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, buffer.texture,0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("framebuffer incomplete");
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glBindTexture(GL_TEXTURE_2D,0);
	return buffer;
}
void CMyApp::SetUniforms(){
 

	glUniform1ui(ul("triangleCount"), (cubeGPU.indexBuffer.GetCount() / 3));

	glUniform2iv(ul("texSize"),1,glm::value_ptr(m_CarSizes[m_active_car_img]));
	glUniform1i(ul("vecSize"),m_vec_tex_size);
	glUniform1i(ul("mode"),m_mode);
	glUniform1i(ul("kernelSize"), static_cast<GLint>(m_kernel_size));
	glUniform1f(ul("sigma"),m_sigma);
	glUniform1f(ul("t_lower"),m_threshold_lower);
	glUniform1f(ul("t_upper"),m_threshold_upper);
	glUniform1i(ul("last_hyst"), m_last_hyst);
	glUniform1ui(ul("seed"), m_seed);
	glUniform1f(ul("lineTh"), m_line_th);
	glUniform1f(ul("lineTh2"), m_line_thth);
	
	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_usedCamera.GetViewProj())); 	
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(glm::translate(m_usedCamera.GetEye())));


	
}
void CMyApp::RenderMeshToTex(const FrameBuffer& target, int w, int h){
	glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
	glViewport(0,0, w, h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(m_multipleObjProgram);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_CarIDs[m_active_car_img]);
	glUniform1i(glGetUniformLocation(m_multipleObjProgram,"texImage"),0);

	CMyApp::SetUniforms();

	m_cubeTransforms.Bind(SHADER_STORAGE,3);
	cubeGPU.Bind();
	glDrawMeshTasksNV(0,1);
	cubeGPU.UnBind();
	m_cubeTransforms.UnBind();

	glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void CMyApp::DrawPass( GLuint srcTex, GLuint destFbo){
	
	int w, h;
	bool depth_test = glIsEnabled(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);;
	if (destFbo == 0){
		w = m_viewPortX;
		h = m_viewportY;
	}
	else if (m_draw_vec){
		w = m_vec_tex_size;
		h = m_vec_tex_size;
	} 
	else {
		w = m_CarSizes[m_active_car_img].x;
		h = m_CarSizes[m_active_car_img].y;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, destFbo);
	glViewport(0,0,w,h);
	if (destFbo != 0){
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glUseProgram(m_flipperProgram);

	CMyApp::SetUniforms();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTex);
	glUniform1i(glGetUniformLocation(m_flipperProgram,"srcImg"), 0);
	
	glBindVertexArray(m_frameVao);
	glDrawArrays(GL_TRIANGLES, 0,3);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	if (depth_test){
		glEnable(GL_DEPTH_TEST);
	}
}

void CMyApp::DrawMainScene()
{
	std::array<CMyApp::FrameBuffer,3> buff = m_frames[m_active_car_img];
	int base_seed = m_seed;

	int n = -1;

	if (m_settings_changed > 0){
		m_settings_changed--;
		m_mode = M_COPY;
		m_draw_vec = false;
		CMyApp::DrawPass(m_CarIDs[m_active_car_img], buff[(n+1)%2].fbo);
		n++;

		if (m_disp_mode >= M_AVG){
			m_mode = M_AVG;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}

		if (m_disp_mode >= M_GAUSS){
			m_mode = M_GAUSS;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode ==M_SOBEL_GRAD){
			m_mode = M_SOBEL_GRAD;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode ==M_SOBEL_DIR){
			m_mode = M_SOBEL_DIR;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode ==M_SOBEL_NORM){
			m_mode = M_SOBEL_NORM;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode >= M_SOBEL){
			m_mode = M_SOBEL;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode >= M_NON_MAX_SUPRESS){
			m_mode = M_NON_MAX_SUPRESS;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode >= M_THRESHOLD){
			m_mode = M_THRESHOLD;
			CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
			n++;
		}
		if (m_disp_mode >= M_HYSTERESIS){
			m_mode = M_HYSTERESIS;
			m_last_hyst = 1;
			for(int i = 0; i <= m_hysteresis_iter; i++){
				m_last_hyst = m_hysteresis_iter-i;
				CMyApp::DrawPass(buff[n%2].texture, buff[(n+1)%2].fbo);
				n++;
			}
		}
		if (m_disp_mode >= M_RANDOM_POINTS){
			m_draw_vec = true;
			m_mode = M_RANDOM_POINTS;
			CMyApp::DrawPass(buff[n%2].texture, m_vec_texes[(n+1)%2].fbo);
			n++;
			m_seed++;
		}
		if (m_disp_mode == M_POINTS_DISP){
			m_draw_vec = false;
			m_mode = M_POINTS_DISP;
			CMyApp::DrawPass(m_vec_texes[n%2].texture, buff[(n+1)%2].fbo);
			n++;
			m_seed++;
		}
		if (m_disp_mode >= M_VECTOR_GEN){
			m_mode = M_VECTOR_GEN;
			CMyApp::DrawPass(m_vec_texes[n%2].texture, m_vec_texes[(n+1)%2].fbo);
			n++;
			m_seed++;
		}

		if (m_disp_mode >= M_LINE_GEN){
			m_mode = M_LINE_GEN;
			CMyApp::DrawPass(m_vec_texes[n%2].texture, m_vec_texes[(n+1)%2].fbo);
			n++;
			m_seed++;
		}
		if (m_disp_mode == M_LINE_DISP){
			m_draw_vec = false;
			m_mode = M_LINE_DISP;
			CMyApp::DrawPass(m_vec_texes[n%2].texture, buff[(n+1)%2].fbo);
			n++;
			m_seed++;
		}
		m_mode = M_COPY;
		CMyApp::DrawPass(m_vec_texes[n%2].texture, m_vec_texes[2].fbo);
		CMyApp::DrawPass(buff[n%2].texture, buff[2].fbo);
	}




	m_mode = M_COPY;
	if (m_draw_vec){
		CMyApp::DrawPass(m_vec_texes[2].texture, 0);
	}
	else{
		CMyApp::DrawPass(buff[2].texture, 0);
	}


	m_seed = base_seed;
	
}


void CMyApp::Render()
{
	// Clear the framebuffer (GL_COLOR_BUFFER_BIT) and the depth buffer (GL_DEPTH_BUFFER_BIT)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawMainScene(); 

	//DrawSkybox();
	//DrawAxes();
}

void CMyApp::RenderGUI()
{
	// ImGui DemoWindow
	
	if (ImGui::BeginMainMenuBar())
	{
		//
		// FPS calculation
		//
		{
			static float refresh_time = 0.1f;
			static float timer = 0;
			static int   frameCount = 0;
			static float fps = 0;

			timer += static_cast<float>(m_delta_time);
			++frameCount;
			if (timer > refresh_time) {
				fps = frameCount / timer;
				timer = 0;
				frameCount = 0;
			}

			ImGui::PushItemWidth(150.0f);
			if(ImGui::SliderInt("mode",&m_disp_mode, M_COPY, M_MAXVAL)){
				m_settings_changed = 2;
			}
			ImGui::Separator();

			ImGui::PushItemWidth(150.0f);
			if(ImGui::SliderInt("image Id",&m_active_car_img, 0, m_car_img_cnt-1)){
				m_settings_changed = 2;
			}
			ImGui::Separator();
					
			if (ImGui::BeginMenu("PostProcessing settings")){
				
				if(ImGui::SliderInt("Kernel size",&m_kernel_size, 0, 20)){
					m_settings_changed = 2;
				}

				if(ImGui::SliderFloat("Sigma",&m_sigma, 0.01, 2)){
					m_settings_changed = 2;
				}

				if(ImGui::SliderFloat("Lower",&m_threshold_lower, 0, 1)){
					m_settings_changed = 2;
				}

				if(ImGui::SliderFloat("Upper",&m_threshold_upper, 0, 1)){
					m_settings_changed = 2;
				}

				if(ImGui::SliderInt("Hyst iterations",&m_hysteresis_iter, 1, 1000)){
					m_settings_changed = 2;
				}

				if(ImGui::SliderInt("Point seed",&m_seed, 1, 1000)){
					m_settings_changed = 2;
				}
				if(ImGui::SliderFloat("line threshold",&m_line_th, 0, .3)){
					m_settings_changed = 2;
				}
				if(ImGui::SliderFloat("line threshold 2",&m_line_thth, 0, 5)){
					m_settings_changed = 2;
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();
			ImGui::Text("FPS: %d", static_cast<int>(fps));

		}
		ImGui::EndMainMenuBar();
	}

	// We set the size and position when we first run the program
	// (when the window is not in th imgui.ini file)
	ImGui::SetNextWindowPos(ImVec2(35, 85), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(800, 150), ImGuiCond_FirstUseEver);

}

GLint CMyApp::ul(const char* uniformName) noexcept
{
	GLuint programID = 0;

	// Get the current program ID
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
	glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&programID));
	// Knowing the program and the name of the uniform, let's get the location!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
	return glGetUniformLocation(programID, uniformName);
}

// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL2/SDL_Keysym
// https://wiki.libsdl.org/SDL2/SDL_Keycode
// https://wiki.libsdl.org/SDL2/SDL_Keymod

void CMyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{
	if (key.repeat == 0) // Triggers only once when held
	{
		if (key.key == SDLK_F5 && key.key & SDL_KMOD_CTRL) // CTRL + F5
		{
			CleanShaders();
			InitShaders();
		}
		if (key.key == SDLK_F1) // F1
		{
			GLint polygonModeFrontAndBack[2] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv(GL_POLYGON_MODE, polygonModeFrontAndBack); // Query the current polygon mode. It gives the front and back modes separately.
			GLenum polygonMode = (polygonModeFrontAndBack[0] != GL_FILL ? GL_FILL : GL_LINE); // Switch between FILL and LINE
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode); // Set the new polygon mode
		}
	}
	m_cameraManipulator.KeyboardDown(key);
}

void CMyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
	m_cameraManipulator.KeyboardUp(key);
}

// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

void CMyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
	m_cameraManipulator.MouseMove(mouse);
}

// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

void CMyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
{
}

// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

void CMyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
	m_cameraManipulator.MouseWheel(wheel);
}

// New window size
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);

	m_viewPortX = _w;
	m_viewportY = _h;

	m_usedCamera.SetAspect(static_cast<float>(_w) / _h);
}

// Other SDL events
// https://wiki.libsdl.org/SDL2/SDL_Event

void CMyApp::OtherEvent(const SDL_Event& ev)
{
}
