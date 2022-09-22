#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include "vulkan_context.h"
#include "vulkan_texture.h"
#include "vulkan_sampler.h"
#include "vulkan_command_buffer.h"
#include "vulkan_shader.h"
#include "shaders/vk_kernel_sources.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader_s.h"
#include "shaders/gl_shader_sources.h"

void window_resize_callback(GLFWwindow* window, int width, int height);
void window_pos_callback(GLFWwindow* window, int posx, int posy);
void processInput(GLFWwindow *window);
int UpdateSharedTextures(int width, int height);
GLuint GetGLExternalTexture(const HANDLE mem_hnd, size_t mem_size, GLenum internal_format, int width, int height);

std::shared_ptr<VulkanContext> g_vk_context;
std::shared_ptr<VulkanTexture> g_vk_src_tex;
std::shared_ptr<VulkanTexture> g_vk_dst_tex;
std::shared_ptr<VulkanComputePipeline> g_vk_pipeline;
std::shared_ptr<VulkanCommandBuffer> g_vk_cmd_buf;
std::shared_ptr<VulkanSemaphore> g_vk_ext_semaphore0;
std::shared_ptr<VulkanSemaphore> g_vk_ext_semaphore1;

std::unique_ptr<Shader> g_render2screen_shader;
std::unique_ptr<Shader> g_render2tex_shader;
GLuint g_gl_ext_semaphore0;
GLuint g_gl_ext_semaphore1;
GLuint g_gl_ext_tex0;
GLuint g_gl_ext_tex1;
GLuint rtt_fbo;
GLuint rtt_src_tex;
GLuint rtt_depth_buf;
GLuint VBO, VAO, EBO;
std::map<GLuint, GLuint> g_gl_ext_texs;

int image_w, image_h;
GLint roi_id;

int main(int argc, char** argv) {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int win_w = 1600;
    const int win_h = 900; 
    GLFWwindow *window = glfwCreateWindow(win_w, win_h, "VulkanGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // glfwSetWindowRefreshCallback(window, window_resize_callback);
    glfwSetWindowSizeCallback(window, window_resize_callback);
    glfwSetWindowPosCallback(window, window_pos_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Load vulkan
    g_vk_context = std::make_shared<VulkanContext>();
    CHECK_RETURN_IF(0 != g_vk_context->Init(), -1);
    // create vulkan shader
    auto vk_shader = std::make_shared<VulkanShader>(g_vk_context);
    CHECK_RETURN_IF(0 != vk_shader->CreateShader(vk_copy_kernel_comp, vk_copy_kernel_comp_len), -1);

    // create vk-gl shared semaphore
    HANDLE ext_vk_signal_semaph_hnd;
    g_vk_ext_semaphore0 = std::make_shared<VulkanSemaphore>(g_vk_context);
    CHECK_RETURN_IF(0 != g_vk_ext_semaphore0->CreateWin32ExternalSemaphore(ext_vk_signal_semaph_hnd), -1);
    glGenSemaphoresEXT(1, &g_gl_ext_semaphore0);
	glImportSemaphoreWin32HandleEXT(g_gl_ext_semaphore0, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, ext_vk_signal_semaph_hnd);
    auto gl_err = glGetError();
    if (gl_err != GL_NO_ERROR) {
        std::cout <<"glImportSemaphoreWin32HandleEXT failed:" << gl_err;
        return gl_err;
    }
    g_vk_ext_semaphore1 = std::make_shared<VulkanSemaphore>(g_vk_context);
    CHECK_RETURN_IF(0 != g_vk_ext_semaphore1->CreateWin32ExternalSemaphore(ext_vk_signal_semaph_hnd), -1);
    glGenSemaphoresEXT(1, &g_gl_ext_semaphore1);
	glImportSemaphoreWin32HandleEXT(g_gl_ext_semaphore1, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, ext_vk_signal_semaph_hnd);
    gl_err = glGetError();
    if (gl_err != GL_NO_ERROR) {
        std::cout <<"glImportSemaphoreWin32HandleEXT failed:" << gl_err;
        return gl_err;
    }

    // load and create textures
    // -------------------------    
    // load image from file
    int image_cn;
    // stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *image_file_data = stbi_load("1.jpg", &image_w, &image_h, &image_cn, 4);
    CHECK_RETURN_IF(!image_file_data, -1);

    // create vulkan ext texture
    UpdateSharedTextures(win_w, win_h);

    auto vk_sampler = std::make_shared<VulkanSampler>(g_vk_context);
    CHECK_RETURN_IF(0 != vk_sampler->CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE), -1);

    g_vk_pipeline = std::make_shared<VulkanComputePipeline>(g_vk_context);
    g_vk_pipeline->Bind(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, g_vk_src_tex->DescriptorInfo());
    g_vk_pipeline->Bind(1, VK_DESCRIPTOR_TYPE_SAMPLER, vk_sampler->DescriptorInfo());
    g_vk_pipeline->Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, g_vk_dst_tex->DescriptorInfo());
    CHECK_RETURN_IF(0 != g_vk_pipeline->CreatePipeline(vk_shader), -1);

    g_vk_cmd_buf = std::make_shared<VulkanCommandBuffer>(g_vk_context);
    CHECK_RETURN_IF(0 != g_vk_cmd_buf->Create(), -1);
    g_vk_cmd_buf->Begin();
    g_vk_cmd_buf->BindPipeline(g_vk_pipeline);
    g_vk_cmd_buf->Dispatch(win_w, win_h);
    g_vk_cmd_buf->End();
    g_vk_cmd_buf->AddWaitSemaphore(g_vk_ext_semaphore0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    g_vk_cmd_buf->AddSignalSemaphore(g_vk_ext_semaphore1);

    // // render to texture
    glGenFramebuffers(1, &rtt_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rtt_fbo);
    // 生成纹理
    glGenTextures(1, &rtt_src_tex);
    glBindTexture(GL_TEXTURE_2D, rtt_src_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image_w, image_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_file_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
	glGenRenderbuffers(1, &rtt_depth_buf);
	glBindRenderbuffer(GL_RENDERBUFFER, rtt_depth_buf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, win_w, win_h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rtt_depth_buf);

    // 将目标纹理到当前绑定的帧缓冲对象
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_gl_ext_tex0, 0);

    // Set the list of draw buffers.
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

	// Always check that our framebuffer is ok
    CHECK_RETURN_IF((glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE), -1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // build and compile our shader zprogram
    g_render2tex_shader = std::unique_ptr<Shader>(new Shader(render_texture_vs, render_image_fs));
    g_render2tex_shader->use();
    g_render2tex_shader->setInt("texture1", 0);
    roi_id = glGetUniformLocation(g_render2tex_shader->ID, "roi");

    g_render2screen_shader = std::unique_ptr<Shader>(new Shader(render_image_vs, render_image_fs));
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    g_render2screen_shader->use(); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    // glUniform1i(glGetUniformLocation(g_render2screen_shader->ID, "texture1"), 0);
    g_render2screen_shader->setInt("texture1", 0);

    // render loop
    // -----------
    int loop_cnt = 0;
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);

        window_resize_callback(window, g_vk_dst_tex->Width(), g_vk_dst_tex->Height());

        // glfw: poll IO events (keys pressed/released, mouse moved etc.)
        // On some platforms(windows), a window move, resize or menu operation will cause event processing to block
        glfwPollEvents();
    }
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    for (auto ext_tex : g_gl_ext_texs) {
        glDeleteMemoryObjectsEXT(1, &(ext_tex.second));
        glDeleteTextures(1, &(ext_tex.first));
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    stbi_image_free(image_file_data);

    printf("main exit.\n");

    return 0;
}

void render_to_texture(int posx, int posy, int width, int height) {
    glViewport(0, 0, width, height);
    g_render2tex_shader->use();
    glBindFramebuffer(GL_FRAMEBUFFER, rtt_fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rtt_depth_buf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rtt_depth_buf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_gl_ext_tex0, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rtt_src_tex);
    glUniform4f(roi_id, float(posx)/image_w, float(posy)/image_h, float(width)/image_w, float(height)/image_h);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    GLenum layout0 = GL_LAYOUT_GENERAL_EXT;
    glSignalSemaphoreEXT(g_gl_ext_semaphore0, 0, nullptr, 1, &g_gl_ext_tex0, &layout0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_to_screen(int width, int height) {
    glViewport(0, 0, width, height);

    GLenum layout1 = GL_LAYOUT_SHADER_READ_ONLY_EXT;
    glWaitSemaphoreEXT(g_gl_ext_semaphore1, 0, nullptr, 1, &g_gl_ext_tex1, &layout1);
    auto gl_err = glGetError();
    if (gl_err != GL_NO_ERROR) {
        std::cout << "glWaitSemaphoreEXT failed:" << gl_err;
    }

    // render container
    g_render2screen_shader->use();
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl_ext_tex1);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void window_pos_callback(GLFWwindow* window, int posx, int posy) {
    int win_x = posx;
    int win_y = posy;
    int win_w;
    int win_h;
    glfwGetWindowSize(window, &win_w, &win_h);
    if ((win_w == g_vk_dst_tex->Width()) || (win_h == g_vk_dst_tex->Height())) {
        render_to_texture(win_x, win_y, win_w, win_h);

        g_vk_cmd_buf->Submit();

        render_to_screen(win_w, win_h);

        glfwSwapBuffers(window);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void window_resize_callback(GLFWwindow* window, int width, int height) {    
    int win_x;
    int win_y;
    int win_w = width;
    int win_h = height;
    // glfwGetWindowSize(window, &win_w, &win_h);
    glfwGetWindowPos(window, &win_x, &win_y);    

    if ((win_w != g_vk_dst_tex->Width()) || (win_h != g_vk_dst_tex->Height())) {
        glDeleteMemoryObjectsEXT(1, &(g_gl_ext_texs[g_gl_ext_tex0]));
        glDeleteTextures(1, &g_gl_ext_tex0);
        g_gl_ext_texs.erase(g_gl_ext_tex0);
        
        glDeleteMemoryObjectsEXT(1, &(g_gl_ext_texs[g_gl_ext_tex1]));
        glDeleteTextures(1, &g_gl_ext_tex1);
        g_gl_ext_texs.erase(g_gl_ext_tex1);

        CHECK_RETURN_IF(0 != UpdateSharedTextures(win_w, win_h), CHECK_VOID);

        g_vk_pipeline->Bind(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, g_vk_src_tex->DescriptorInfo());
        g_vk_pipeline->Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, g_vk_dst_tex->DescriptorInfo());
        g_vk_pipeline->UpdateBindings();

        g_vk_cmd_buf->Begin();
        g_vk_cmd_buf->BindPipeline(g_vk_pipeline);
        g_vk_cmd_buf->Dispatch(win_w, win_h);
        g_vk_cmd_buf->End();
    }

    render_to_texture(win_x, win_y, win_w, win_h);

    g_vk_cmd_buf->Submit();
    // g_vk_cmd_buf->WaitComplete();

    render_to_screen(win_w, win_h);

    glfwSwapBuffers(window);
}

int UpdateSharedTextures(int width, int height) {
    HANDLE mem_hnd;
    size_t mem_size;
    int vk_ret = 0;
    g_vk_src_tex = std::make_shared<VulkanTexture>(g_vk_context);
    vk_ret = g_vk_src_tex->CreateWin32GlSharedTexture(VK_FORMAT_R8G8B8A8_UNORM,
                                                      width,
                                                      height,
                                                      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                      VK_IMAGE_TILING_OPTIMAL,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                                      mem_hnd,
                                                      mem_size);
    CHECK_RETURN_IF(0 != vk_ret, vk_ret);
    g_gl_ext_tex0 = GetGLExternalTexture(mem_hnd, mem_size, GL_RGBA8, g_vk_src_tex->Width(), g_vk_src_tex->Height());
    CHECK_RETURN_IF(0 == g_gl_ext_tex0, -1);

    g_vk_dst_tex = std::make_shared<VulkanTexture>(g_vk_context);
    vk_ret = g_vk_dst_tex->CreateWin32GlSharedTexture(VK_FORMAT_R8G8B8A8_UNORM,
                                                      width,
                                                      height,
                                                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                      VK_IMAGE_TILING_OPTIMAL,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                                      mem_hnd,
                                                      mem_size);
    CHECK_RETURN_IF(0 != vk_ret, vk_ret);
    g_gl_ext_tex1 = GetGLExternalTexture(mem_hnd, mem_size, GL_RGBA8, g_vk_dst_tex->Width(), g_vk_dst_tex->Height());
    CHECK_RETURN_IF(0 == g_gl_ext_tex1, -1);
    return 0;
}

GLuint GetGLExternalTexture(const HANDLE mem_hnd, size_t mem_size, GLenum internal_format, int width, int height) {
    GLuint gl_tex = 0;
    GLuint mem = 0;
    GLenum gl_err = GL_NO_ERROR;
    do {
        glCreateMemoryObjectsEXT(1, &mem);

		glImportMemoryWin32HandleEXT(mem, mem_size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, mem_hnd);
        gl_err = glGetError();
        if (gl_err != GL_NO_ERROR) {
            std::cout << __FUNCTION__ << ": glImportMemoryWin32HandleEXT failed:" << gl_err << std::endl;
            break;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &gl_tex);

        glTextureStorageMem2DEXT(gl_tex, 1, internal_format, width, height, mem, 0);
        gl_err = glGetError();
        if (gl_err != GL_NO_ERROR) {
            std::cout << __FUNCTION__ << ": glTextureStorageMem2DEXT failed:" << gl_err << std::endl;
            glDeleteTextures(1, &gl_tex);
            gl_tex = 0;
            break;
        }
    } while(0);

    if ((0 == gl_tex) && (0 != mem)) {
        glDeleteMemoryObjectsEXT(1, &mem);
        mem = 0;
    }

    if (0 != gl_tex) {
        g_gl_ext_texs[gl_tex] = mem;
    }
    
    glBindTexture(GL_TEXTURE_2D, gl_tex);
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return gl_tex;
}