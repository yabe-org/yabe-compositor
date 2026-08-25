#pragma once

#include <GLES3/gl32.h>
#include <sys/types.h>
#include <wayland-server-core.h>

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "common/common.hpp"

namespace yabe::proto::zwin::gles_v32 {
class Buffer;
class Program;
class RenderingUnit;
class Sampler;
class Texture;
class VertexArray;

class BaseTechnique {
 public:
  BaseTechnique() = default;
  DISABLE_MOVE_AND_COPY(BaseTechnique);
  ~BaseTechnique() = default;

  void bind_program(const std::shared_ptr<Program>& program);
  void bind_vertex_array(const std::shared_ptr<VertexArray>& vertex_array);
  void bind_texture(GLuint binding, std::string name,
      const std::shared_ptr<Texture>& texture, GLenum target,
      const std::shared_ptr<Sampler>& sampler);
  void set_uniform_vector(GLuint location, std::string name, uint32_t type,
      uint32_t size, GLsizei count, std::vector<std::byte> value);
  void set_uniform_matrix(GLuint location, std::string name, uint32_t col,
      uint32_t row, GLsizei count, GLboolean transpose,
      std::vector<std::byte> value);
  void draw_arrays(GLenum mode, GLint first, GLsizei count);
  void draw_elements(GLenum mode, GLsizei count, GLenum type, off_t offset,
      const std::shared_ptr<Buffer>& buffer);

  void commit();
  void draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& model_mat);

 private:
  struct TextureBinding {
    std::string            name;
    std::weak_ptr<Texture> texture;
    GLenum                 target;
    std::weak_ptr<Sampler> sampler;
  };
  struct UniformVector {
    GLint                  location;
    std::string            name;
    uint32_t               type;
    uint32_t               size;
    GLsizei                count;
    std::vector<std::byte> value;
  };
  struct UniformMatrix {
    GLint                  location;
    std::string            name;
    uint32_t               col;
    uint32_t               row;
    GLsizei                count;
    GLboolean              transpose;
    std::vector<std::byte> value;
  };
  struct DrawArrays {
    GLenum  mode;
    GLint   first;
    GLsizei count;
  };
  struct DrawElements {
    GLenum                mode;
    GLsizei               count;
    GLenum                type;
    off_t                 offset;
    std::weak_ptr<Buffer> buffer;
  };
  using Uniform = std::variant<UniformVector, UniformMatrix>;
  using Draw    = std::variant<DrawArrays, DrawElements>;
  struct {
    std::weak_ptr<Program>                     program;
    std::weak_ptr<VertexArray>                 vertex_array;
    std::unordered_map<GLuint, TextureBinding> textures;
    std::vector<Uniform>                       uniforms;
    std::vector<Draw>                          draws;
  } pending_, current_;

  std::mutex mutex_;
};

void create_base_technique(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<RenderingUnit>& rendering_unit);
}  // namespace yabe::proto::zwin::gles_v32
