#include "gles-v32.hpp"

#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include "base-technique.hpp"
#include "buffer.hpp"
#include "core/server.hpp"
#include "program.hpp"
#include "protocols/zwin/shm/shm-buffer.hpp"
#include "rendering-unit.hpp"
#include "sampler.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "vertex-array.hpp"

namespace yabe::proto::zwin::gles_v32 {
namespace {
constexpr struct zwn_gles_v32_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .create_rendering_unit =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* virtual_object_resource) {
          const auto virtual_object =
              core::server::get().resources.get<VirtualObject>(
                  virtual_object_resource);
          if (!virtual_object) {
            wl_client_post_implementation_error(client,
                "zwn_gles_v32.create_rendering_unit: invalid "
                "zwn_virtual_object");
            return;
          }
          create_rendering_unit(
              client, wl_resource_get_version(resource), id, virtual_object);
        },
    .create_gl_buffer =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_buffer(client, wl_resource_get_version(resource), id);
        },
    .create_gl_shader =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* buffer_resource, uint32_t type) {
          GLenum shader_type = type;
          switch (type) {
            // while `type` at create_gl_shader is zwn_gl_shader.type,
            // zukou uses OpenGL enum value, so accept them
            case GL_FRAGMENT_SHADER:
            case GL_VERTEX_SHADER:
              break;
            case ZWN_GL_SHADER_TYPE_FRAGMENT_SHADER:
              shader_type = GL_FRAGMENT_SHADER;
              break;
            case ZWN_GL_SHADER_TYPE_VERTEX_SHADER:
              shader_type = GL_VERTEX_SHADER;
              break;
            default:
              wl_resource_post_error(resource, ZWN_GLES_V32_ERROR_INVALID_ENUM,
                  "zwn_gl_shader: invalid type");
              return;
          }
          auto* buffer = shm::get_buffer(buffer_resource);
          if (!buffer) {
            wl_client_post_implementation_error(
                client, "zwn_gles_v32.create_gl_shader: invalid zwn_buffer");
            return;
          }
          auto* data = shm::get_buffer_data(buffer);
          auto  size = shm::get_buffer_size(buffer);
          shm::buffer_begin_access(buffer);
          std::string source(static_cast<char*>(data), size);
          shm::buffer_end_access(buffer);
          create_shader(client, wl_resource_get_version(resource), id,
              std::move(source), shader_type);
        },
    .create_gl_program =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_program(client, wl_resource_get_version(resource), id);
        },
    .create_gl_texture =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_texture(client, wl_resource_get_version(resource), id);
        },
    .create_gl_sampler =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_sampler(client, wl_resource_get_version(resource), id);
        },
    .create_gl_vertex_array =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_vertex_array(client, wl_resource_get_version(resource), id);
        },
    .create_gl_base_technique =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* rendering_unit_resource) {
          const auto rendering_unit =
              core::server::get().resources.get<RenderingUnit>(
                  rendering_unit_resource);
          if (!rendering_unit) {
            wl_client_post_implementation_error(client,
                "zwn_gles_v32.create_gl_base_technique: invalid "
                "zwn_rendering_unit");
            return;
          }
          create_base_technique(
              client, wl_resource_get_version(resource), id, rendering_unit);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_gles_v32_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::zwin::gles_v32
