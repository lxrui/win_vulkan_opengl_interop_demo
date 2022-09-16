#ifndef __VK_KERNEL_SOURCES_H__
#define __VK_KERNEL_SOURCES_H__

static const char vk_copy_kernel[] = R"""(
#version 450
layout (local_size_x_id = 1, local_size_y_id = 2, local_size_z_id = 3) in;
layout (binding = 0)            uniform texture2D src_tex;  //RGBA8
layout (binding = 1)            uniform sampler sl;
layout (binding = 2, rgba8)     uniform image2D dst_tex;    //RGBA8

#define GID gl_GlobalInvocationID
#define IGID ivec2(gl_GlobalInvocationID)

void main() {
    uvec2 dst_size = uvec2(imageSize(dst_tex));
    if((GID.x >= dst_size.x) || (GID.y >= dst_size.y)) return;
    vec2 norm_coord = (vec2(GID) + vec2(0.5f)) / vec2(dst_size);
    vec4 src_val = texture(sampler2D(src_tex, sl), norm_coord);
    imageStore(dst_tex, IGID, src_val);
}
)""";

#endif