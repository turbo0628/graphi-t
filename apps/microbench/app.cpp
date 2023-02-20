#include <iostream>
#include <string>
#include "gft/log.hpp"
#include "gft/vk.hpp"
#include "gft/glslang.hpp"
#include "gft/platform/macos.hpp"
#include "gft/platform/windows.hpp"

using namespace liong;
using namespace vk;
using namespace fmt;

void copy_buf2host(
  scoped::Buffer& src,
  void* dst,
  size_t size
) {
  if (size == 0) {
    L_WARN("zero-sized copy is ignored");
    return;
  }
  L_ASSERT(src.size() >= size, "src buffer size is too small");
  scoped::MappedBuffer mapped(src, L_MEMORY_ACCESS_READ_BIT);
  std::memcpy(dst, (const void*)mapped, size);
}
void copy_host2buf(
  const void* src,
  scoped::Buffer& dst,
  size_t size
) {
  if (size == 0) {
    L_WARN("zero-sized copy is ignored");
    return;
  }
  L_ASSERT(dst.size() >= size, "dst buffser size is too small");
  scoped::MappedBuffer mapped(dst, L_MEMORY_ACCESS_WRITE_BIT);
  std::memcpy((void*)mapped, mapped, size);
}



void dbg_enum_dev_descs() {
  uint32_t ndev = 0;
  for (;;) {
    auto desc = desc_dev(ndev);
    if (desc.empty()) { break; }
    L_INFO("device #", ndev, ": ", desc);
    ++ndev;
  }
}
void dbg_dump_spv_art(
  const std::string& prefix,
  glslang::ComputeSpirvArtifact art
) {
  liong::util::save_file(
    (prefix + ".comp.spv").c_str(),
    art.comp_spv.data(),
    art.comp_spv.size() * sizeof(uint32_t));
}
void dbg_dump_spv_art(
  const std::string& prefix,
  glslang::GraphicsSpirvArtifact art
) {
  liong::util::save_file(
    (prefix + ".vert.spv").c_str(),
    art.vert_spv.data(),
    art.vert_spv.size() * sizeof(uint32_t));
  liong::util::save_file(
    (prefix + ".frag.spv").c_str(),
    art.frag_spv.data(),
    art.frag_spv.size() * sizeof(uint32_t));
}

std::string get_shader_string(int repeats) {
  char src[4096];
  sprintf(src, R"(
    #version 460 core

    layout(local_size_x_id = 8, local_size_y_id = 8, local_size_z_id = 8) in; 
    layout(set = 0, binding = 0) uniform sampler3D img;
    layout(set = 0, binding = 1, rgba8) writeonly uniform image3D img2;
    void main() {
      vec3 coord = vec3(gl_GlobalInvocationID) / vec3(gl_NumWorkGroups * gl_WorkGroupSize);

      vec4 col = vec4(0.0f,0.0f,0.0f,0.0f);
      for (int i = 0; i < %d; ++i) {
        vec4 color = texture(img, coord);
        col += color;
      }
      imageStore(img2, ivec3(gl_GlobalInvocationID), col);
    }
  )", repeats);
  return std::string(src);
}

void guarded_main() {
  scoped::GcScope scope;

  dbg_enum_dev_descs();

  scoped::Context ctxt = scoped::ContextBuilder()
    .build();
  scoped::Image img = ctxt.build_img()
                          .width(128)
                          .height(128)
                          .depth(128)
                          .fmt(fmt::Format::L_FORMAT_R8G8B8A8_UNORM)
                          .sampled()
                          .storage()
                          .build();
  scoped::Image img2 = ctxt.build_img()
                           .width(128)
                           .height(128)
                           .depth(128)
                           .fmt(fmt::Format::L_FORMAT_R8G8B8A8_UNORM)
                           .sampled()
                           .storage()
                           .build();

  int repeats = 200;
  const char* warmup_src = get_shader_string(10).c_str();
  const char* benchmark_src = get_shader_string(repeats).c_str();

  std::vector<uint32_t> src_spv = glslang::compile_comp(warmup_src, "main").comp_spv;
  std::vector<uint32_t> benchmark_src_spv = glslang::compile_comp(benchmark_src, "main").comp_spv;

  scoped::Task task = ctxt.build_comp_task()
      .workgrp_size(8, 8, 8)
      .comp(src_spv)
      .comp_entry_name("main")
      .rsc(L_RESOURCE_TYPE_SAMPLED_IMAGE)
      .rsc(L_RESOURCE_TYPE_STORAGE_IMAGE)
      .build();

  scoped::Invocation invoke =  task.build_comp_invoke()
      .rsc(img.view())
      .rsc(img2.view())
      .workgrp_count(128, 128, 128)
      .build();
  // scoped::Transaction transact = invoke.submit();
  // transact.wait();
  scoped::Task benchmark_task = ctxt.build_comp_task()
      .workgrp_size(8, 8, 8)
      .comp(benchmark_src_spv)
      .comp_entry_name("main")
      .rsc(L_RESOURCE_TYPE_SAMPLED_IMAGE)
      .rsc(L_RESOURCE_TYPE_STORAGE_IMAGE)
      .build();

  scoped::Invocation benchmark_invoke =  task.build_comp_invoke()
      .is_timed()
      .rsc(img.view())
      .rsc(img2.view())
      .workgrp_count(128, 128, 128)
      .build();
  scoped::Transaction benchmark_transact = benchmark_invoke.submit();
  benchmark_transact.wait();
  double time = benchmark_invoke.get_time_us();
  std::cout << time / 1e3 / repeats << "ms" << std::endl;
}


int main(int argc, char** argv) {
  try {
    vk::initialize();
    glslang::initialize();

    guarded_main();
  } catch (const std::exception& e) {
    L_ERROR("application threw an exception");
    L_ERROR(e.what());
    L_ERROR("application cannot continue");
  } catch (...) {
    L_ERROR("application threw an illiterate exception");
  }

  return 0;
}
