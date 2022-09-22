#ifndef __GL_SHADER_SOURCES_H__
#define __GL_SHADER_SOURCES_H__

static const char render_image_vs[] = R"""(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	TexCoord = vec2(aTexCoord.x, 1.0f - aTexCoord.y);
}
)""";

static const char render_texture_vs[] = R"""(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform vec4 roi;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	TexCoord = vec2(aTexCoord.x * roi.z + roi.x, aTexCoord.y * roi.w + roi.y);
}
)""";

static const char render_image_fs[] = R"""(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;

void main()
{
	FragColor = texture(texture1, TexCoord);
}
)""";

#endif  