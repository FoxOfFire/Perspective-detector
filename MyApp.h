#pragma once

// GLM
#include <glm/ext/vector_float2.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

// Utils
#include "GLUtils.hpp"
#include "Camera.h"
#include "CameraManipulator.h"

#include "GLMeshShaderUtils.h"

// Standard
#include <array>


struct SUpdateInfo
{
	float ElapsedTimeInSec = 0.0f;	// Elapsed time since start of the program
	float DeltaTimeInSec = 0.0f;	// Elapsed time since last update
};

class CMyApp
{
public:
	CMyApp();
	~CMyApp();

	bool Init();
	void Clean();

	void Update(const SUpdateInfo&);
	void DrawMainScene();
	void Render();
	void RenderGUI();

	void KeyboardDown(const SDL_KeyboardEvent&);
	void KeyboardUp(const SDL_KeyboardEvent&);
	void MouseMove(const SDL_MouseMotionEvent&);
	void MouseDown(const SDL_MouseButtonEvent&);
	void MouseUp(const SDL_MouseButtonEvent&);
	void MouseWheel(const SDL_MouseWheelEvent&);
	void Resize(int, int);

	void OtherEvent(const SDL_Event&);
protected:
	void SetupDebugCallback();

	//
	// Variables
	//

	float	m_ElapsedTimeInSec		= 0.0f;
	float	m_delta_time			= 0.0f;
	bool	m_lockAxesToCamera		= false;

	enum CameraIds {
		PRIMARY_CAMERA		= 0,
		SECONDARY_CAMERA	= 1
	};

	// Camera
	size_t m_currentCamera = PRIMARY_CAMERA;
	Camera m_usedCamera;
	CameraManipulator m_cameraManipulator;


	int m_viewPortX = 1;
	int m_viewportY = 1;
	//
	// OpenGL stuff
	//

	void DrawAxes();
	void DrawSkybox();
	// Uniform location query
	static GLint ul(const char* uniformName) noexcept;

	// Shader variables
	GLuint m_programID = 0;				// Shader of the objects
	GLuint m_programAxesID = 0;			// Program showing X,Y,Z directions
	GLuint m_programSkyboxID = 0;		// Skybox program

	GLuint m_multipleObjProgram = 0;
	//postProcess
	GLuint m_flipperProgram = 0;

	// Light source
	glm::vec3 m_lightDir = glm::vec3(0, 0, -0.5);
	glm::vec3 m_La = glm::vec3(0.0, 0.0, 0.0);	// Ambient
	glm::vec3 m_Ld = glm::vec3(1.0, 1.0, 1.0);	// Diffuse

	// Shader initialization and termination
	void InitShaders();
	void CleanShaders();
	void InitMeshShaders();
	void CleanMeshShaders();
	void InitAxesShader();
	void CleanAxesShader();
	void InitSkyboxShaders();
	void CleanSkyboxShaders();
	
	void InitFlipperShaders();
	void CleanFlipperShaders();
	// 
	// Geometry variables
	// 
	OGLObject m_Skybox	= {};

	// 
	// Buffers
	// 
	GLImmutableBuffer<glm::mat4> m_cubeTransforms = {};

	//
	// Mesh shader objects
	//
	MeshShaderObject<PaddedVertex> cubeGPU;

	void InitGeometry();
	void CleanGeometry();
	void InitSkyboxGeometry();
	void CleanSkyboxGeometry();

	// Texture variables
	GLuint m_SamplerID = 0;
	GLuint m_MossyTextureID	= 0;
	static const int m_car_img_cnt = 4;
	int m_active_car_img =0;
	std::array<GLuint,m_car_img_cnt> m_CarIDs={0,0,0,0};
	std::array<float,m_car_img_cnt> m_CarAspects={1.0,1.0,1.0,1.0};
	std::array<glm::ivec2,m_car_img_cnt> m_CarSizes={glm::ivec2(100),glm::ivec2(100),glm::ivec2(100),glm::ivec2(100)};
	GLuint m_SkyboxTextureID = 0;

	int m_settings_changed = 2;
	bool m_draw_vec = false;

	int m_kernel_size = 9;
	GLfloat m_sigma = 1;
	GLfloat m_threshold_lower = .25;
	GLfloat m_threshold_upper = .5;
	int m_hysteresis_iter = 200;
	GLuint m_last_hyst = 0;
	GLint m_seed = 0;
	GLfloat m_line_th = .1;
	GLfloat m_line_thth = 1;

	int m_vec_tex_size = 	200;

	//Modes:
	static const int M_COPY = 0;
	static const int M_AVG = M_COPY + 1;
	static const int M_GAUSS = M_AVG + 1;
	static const int M_SOBEL_GRAD = M_GAUSS + 1;
	static const int M_SOBEL_DIR = M_SOBEL_GRAD + 1;
	static const int M_SOBEL_NORM = M_SOBEL_DIR + 1;
	static const int M_SOBEL = M_SOBEL_NORM + 1;
	static const int M_NON_MAX_SUPRESS = M_SOBEL + 1;
	static const int M_THRESHOLD = M_NON_MAX_SUPRESS + 1;
	static const int M_HYSTERESIS = M_THRESHOLD + 1;
	static const int M_RANDOM_POINTS = M_HYSTERESIS + 1;
	static const int M_POINTS_DISP = M_RANDOM_POINTS + 1;
	static const int M_VECTOR_GEN = M_POINTS_DISP + 1;
	static const int M_LINE_GEN = M_VECTOR_GEN + 1;
	static const int M_LINE_DISP = M_LINE_GEN+ 1;

	static const int M_MAXVAL = M_LINE_DISP ;

	int m_mode = M_COPY;
	int m_disp_mode = M_COPY;



	struct FrameBuffer{
		GLuint texture = 0;
		GLuint fbo = 0;
	};
	std::array<std::array<FrameBuffer,3>,m_car_img_cnt> m_frames;
	std::array<FrameBuffer,3> m_vec_texes;
	GLuint m_frameVao;

	FrameBuffer MakeBuffer(int w, int h);
	void SetUniforms();
	void RenderMeshToTex(const FrameBuffer& target, int w, int h);
	void DrawPass( GLuint srcTex, GLuint destFbo);
	void DisplayPostProcessedImage(GLuint tex);

	// Texture initialization and termination
	void InitTextures();
	void CleanTextures();
	void InitSkyboxTextures();
	void CleanSkyboxTextures();
};
