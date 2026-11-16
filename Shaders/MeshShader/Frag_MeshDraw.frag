#version 450

layout(location = 0) out vec4 FragColor;

uniform sampler2D texImage;

in vec4 ms_out_col;
in vec4 ms_out_task_col;
in vec3 ms_out_norm;
in vec2 ms_out_tex;

void main()
{
	FragColor =texture(texImage,ms_out_tex);
}
