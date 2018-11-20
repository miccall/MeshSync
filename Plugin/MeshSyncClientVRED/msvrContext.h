#pragma once

#include <GL/glew.h>
#include "MeshSync/MeshSync.h"
#include "MeshSync/MeshSyncUtils.h"

using ms::float2;
using ms::float3;
using ms::float4;
using ms::quatf;
using ms::float2x2;
using ms::float3x3;
using ms::float4x4;

#define msvrMaxTextureSlots 32

class msvrContext;

msvrContext* msvrGetContext();
void msvrInitializeWidget();


struct vr_vertex
{
    float3 vertex;
    float3 normal;
    float2 uv;
    float4 color;
};

struct TextureRecord
{
    ms::TexturePtr dst;
    bool dirty = true;
    bool used = false;
};

struct FramebufferRecord
{
    GLuint colors[16] = {};
    GLuint depth_stencil = 0;

    bool isMainTarget() const;
};

struct MaterialRecord
{
    int id = ms::InvalidID;
    GLuint program = 0;
    float4 diffuse_color = float4::zero();
    float4 specular_color = float4::zero();
    float bump_scale = 0.0f;
    int color_map = ms::InvalidID;
    int bump_map = ms::InvalidID;
    int specular_map = ms::InvalidID;
    GLuint texture_slots[msvrMaxTextureSlots] = {};

    bool operator==(const MaterialRecord& v) const
    {
        return
            program == v.program &&
            diffuse_color == v.diffuse_color &&
            specular_color == v.specular_color &&
            bump_scale == v.bump_scale &&
            color_map == v.color_map &&
            bump_map == v.bump_map &&
            specular_map == v.specular_map &&
            memcmp(texture_slots, v.texture_slots, sizeof(GLuint)*msvrMaxTextureSlots) == 0;
    }
    bool operator!=(const MaterialRecord& v) const
    {
        return !operator==(v);
    }
    uint64_t checksum() const
    {
        return ms::SumInt32(this, sizeof(*this));
    }
};

namespace ms {

template<>
class IDGenerator<MaterialRecord> : public IDGenerator<void*>
{
public:
    int getID(const MaterialRecord& o)
    {
        auto ck = o.checksum();
        return getIDImpl((void*&)ck);
    }
};

} // namespace ms

struct BufferRecord : public mu::noncopyable
{
    RawVector<char> data, tmp_data;
    void        *mapped_data = nullptr;
    int         stride = 0;
    bool        dirty = false;
    bool        visible = true;
    int         material_id = -1;

    ms::MeshPtr dst_mesh;
};

struct ProgramRecord
{
    struct Uniform
    {
        std::string name;
        ms::MaterialProperty::Type type;
        int size;
    };
    std::map<GLuint, Uniform> uniforms;
    MaterialRecord mrec;
};



struct msvrSettings
{
    ms::ClientSettings client_settings;
    float scale_factor = 100.0f;
    bool auto_sync = false;
    bool sync_delete = true;
    bool sync_camera = true;
    bool sync_textures = true;
    std::string camera_path = "/Main Camera";
};

class msvrContext
{
public:
    msvrContext();
    ~msvrContext();
    msvrSettings& getSettings();
    void send(bool force);

    void onGenTextures(GLsizei n, GLuint * textures);
    void onDeleteTextures(GLsizei n, const GLuint * textures);
    void onActiveTexture(GLenum texture);
    void onBindTexture(GLenum target, GLuint texture);
    void onTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);

    void onGenFramebuffers(GLsizei n, GLuint *ids);
    void onBindFramebuffer(GLenum target, GLuint framebuffer);
    void onDeleteFramebuffers(GLsizei n, GLuint *framebuffers);
    void onFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
    void onFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);

    void onGenBuffers(GLsizei n, GLuint* buffers);
    void onDeleteBuffers(GLsizei n, const GLuint* buffers);
    void onBindBuffer(GLenum target, GLuint buffer);
    void onBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
    void onBindBufferBase(GLenum target, GLuint index, GLuint buffer);
    void onBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
    void onNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizei size, const void *data);
    void onMapBuffer(GLenum target, GLenum access, void *&mapped_data);
    void onMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void *&mapped_data);
    void onUnmapBuffer(GLenum target);
    void onFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);

    void onGenVertexArrays(GLsizei n, GLuint *buffers);
    void onDeleteVertexArrays(GLsizei n, const GLuint *buffers);
    void onBindVertexArray(GLuint buffer);
    void onEnableVertexAttribArray(GLuint index);
    void onDisableVertexAttribArray(GLuint index);
    void onVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

    void onLinkProgram(GLuint program);
    void onDeleteProgram(GLuint program);
    void onUseProgram(GLuint program);
    void onUniform1i(GLint location, GLint v0);
    void onUniform1f(GLint location, GLfloat v0);
    void onUniform1fv(GLint location, GLsizei count, const GLfloat *value);
    void onUniform2fv(GLint location, GLsizei count, const GLfloat *value);
    void onUniform3fv(GLint location, GLsizei count, const GLfloat *value);
    void onUniform4fv(GLint location, GLsizei count, const GLfloat *value);
    void onUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void onUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void onUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

    void onDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
    void onFlush();

protected:
    BufferRecord* getActiveBuffer(GLenum target);
    ProgramRecord::Uniform* findUniform(GLint location);

protected:
    msvrSettings m_settings;

    std::map<GLuint, BufferRecord> m_buffer_records;
    std::vector<GLuint> m_meshes_deleted;

    GLuint m_vb_handle = 0;
    GLuint m_ib_handle = 0;
    GLuint m_ub_handle = 0;
    GLuint m_ub_handles[16] = {};

    int m_active_texture = 0;
    GLuint m_texture_slots[msvrMaxTextureSlots] = {};
    std::map<GLuint, TextureRecord> m_texture_records;

    GLuint m_fb_handle = 0;
    std::map<GLuint, FramebufferRecord> m_framebuffer_records;

    GLuint m_program_handle = 0;
    std::map<GLuint, ProgramRecord> m_program_records;
    std::vector<MaterialRecord> m_material_records;

    bool m_camera_dirty = false;
    float3 m_camera_pos = float3::zero();
    quatf m_camera_rot = quatf::identity();
    float m_camera_fov = 60.0f;
    float m_camera_near = 0.01f;
    float m_camera_far = 100.0f;

    ms::IDGenerator<MaterialRecord> m_material_ids;
    ms::CameraPtr m_camera;
    ms::TextureManager m_texture_manager;
    ms::MaterialManager m_material_manager;
    ms::EntityManager m_entity_manager;

    ms::AsyncSceneSender m_sender;
};