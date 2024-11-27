#include "webgpu_renderer.h"
#include "logstorm/manager.h"
#include <array>
#include <set>
#include <string>
#include <vector>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <imgui/imgui_impl_wgpu.h>
#include <magic_enum/magic_enum.hpp>
#include "vectorstorm/matrix/matrix3.h"
#include "sqrt_constexpr.h"
#include "instance.h"
#include "shaders/default.wgsl.h"


namespace render {

namespace {
mat4f make_projection_matrix(vec2f const &viewport_size) {
  /// Set up a projection matrix based on the FOV and the relevant planes
  enum class fov_mode_type {                                                    // how we set the field of view
    horizontal,                                                                 // field of view measures max horizontal angle of view
    vertical,                                                                   // field of view measures max vertical angle of view
    diagonal,                                                                   // field of view measures max diagonal angle of view
  } fov_mode{fov_mode_type::diagonal};

  float fov_angle{110.0f};                                                      // field of view, in degrees
  float fov_angle_rad{DEG2RAD(fov_angle)};                                      // field of view, in radians - automatically updated from degrees
  float clip_plane_near{1.0f};                                                  // near clip plane
  float clip_plane_far{100'000.0f};                                             // far clip plane

  #pragma GCC diagnostic push
  #pragma GCC diagnostic error "-Wswitch"                                       // enforce exhaustive switch here
  switch(fov_mode) {
  #pragma GCC diagnostic pop
  case fov_mode_type::horizontal:
    {
      float const aspect_ratio{viewport_size.y / viewport_size.x};
      float const right{std::tan(fov_angle_rad * 0.5f) * clip_plane_near};
      float const top{right * aspect_ratio};
      //logger << "DEBUG: horizontal aspect_ratio " << aspect_ratio;
      return mat4f::create_frustum(-right, right, -top, top, clip_plane_near, clip_plane_far);
    }
  case fov_mode_type::vertical:
    {
      float const aspect_ratio{viewport_size.x / viewport_size.y};
      float const top{std::tan(fov_angle_rad * 0.5f) * clip_plane_near};
      float const right{top * aspect_ratio};
      //logger << "DEBUG: vertical aspect_ratio " << aspect_ratio;
      return mat4f::create_frustum(-right, right, -top, top, clip_plane_near, clip_plane_far);
    }
  case fov_mode_type::diagonal:
    {
      float const diagonal{std::tan(fov_angle_rad * 0.5f) * clip_plane_near};
      float const viewport_diagonal{viewport_size.length()};
      float const right{diagonal * (viewport_size.x / viewport_diagonal)};
      float const top{  diagonal * (viewport_size.y / viewport_diagonal)};
      //logger << "DEBUG: diagonal aspect_ratio " << viewport_size.x / viewport_size.y;
      return mat4f::create_frustum(-right, right, -top, top, clip_plane_near, clip_plane_far);
    }
    // no default case, to enforce exhaustive switch
  }
  std::unreachable();
}

template<typename Tcpp, typename Tc>
std::string enum_wgpu_name(Tc enum_in) {
  /// Attempt to interpret an enum into its most human-readable form, with fallbacks for unknown types
  /// Tc is the C API enum (WGPU...), Tcpp is the C++ API enum equivalent (wgpu::...)
  if(auto enum_out_opt{magic_enum::enum_cast<Tcpp>(enum_in)}; enum_out_opt.has_value()) { // first try to cast it to the C++ enum for clearest output
    return std::string{magic_enum::enum_name(*enum_out_opt)};
  }

  if(auto enum_out_opt{magic_enum::enum_cast<Tc>(enum_in)}; enum_out_opt.has_value()) { // fall back to trying the C enum interpretation
   return std::string{magic_enum::enum_name(*enum_out_opt)} + " (C binding only)";
  }

  std::ostringstream oss;
  oss << "unknown enum 0x" << std::hex << enum_in;                              // otherwise output the hex value and an explanatory note
  return oss.str();
}

template<typename Tcpp, typename Tc>
std::string enum_wgpu_name(Tcpp enum_in) {
  /// When passing the C++ version, cast it to the C version
  return enum_wgpu_name<Tcpp, Tc>(static_cast<Tc>(enum_in));
}

}

webgpu_renderer::webgpu_renderer(logstorm::manager &this_logger)
  : logger{this_logger} {
  /// Construct a WebGPU renderer and populate those members that don't require delayed init
  if(!webgpu.instance) throw std::runtime_error{"Could not initialize WebGPU"};

  // find out about the initial canvas size and the current window and doc sizes
  window.viewport_size.assign(emscripten::val::global("window")["innerWidth"].as<unsigned int>(),
                              emscripten::val::global("window")["innerHeight"].as<unsigned int>());
  window.device_pixel_ratio = emscripten::val::global("window")["devicePixelRatio"].as<float>(); // query device pixel ratio using JS
  logger << "WebGPU: Viewport size: " << window.viewport_size << " (device pixels: approx " << static_cast<vec2f>(window.viewport_size) * window.device_pixel_ratio << ")";
  logger << "WebGPU: Device pixel ratio: " << window.device_pixel_ratio << " canvas pixels to 1 device pixel (" << static_cast<unsigned int>(std::round(100.0f * window.device_pixel_ratio)) << "% zoom)";

  // create a surface
  {
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector surface_descriptor_from_canvas;
    surface_descriptor_from_canvas.selector = "#canvas";

    wgpu::SurfaceDescriptor surface_descriptor{
      .nextInChain{&surface_descriptor_from_canvas},
      .label{"Canvas surface"},
    };
    webgpu.surface = webgpu.instance.CreateSurface(&surface_descriptor);
  }
  if(!webgpu.surface) throw std::runtime_error{"Could not create WebGPU surface"};
}

void webgpu_renderer::init(std::function<void(webgpu_data const&)> &&this_postinit_callback, std::function<void()> &&this_main_loop_callback) {
  /// Initialise the WebGPU system
  postinit_callback = this_postinit_callback;
  main_loop_callback = this_main_loop_callback;

  {
    // request an adapter
    wgpu::RequestAdapterOptions adapter_request_options{
      .compatibleSurface{webgpu.surface},
      .powerPreference{wgpu::PowerPreference::HighPerformance},
    };

    webgpu.instance.RequestAdapter(
      &adapter_request_options,
      [](WGPURequestAdapterStatus status_c, WGPUAdapterImpl *adapter_ptr, const char *message, void *data){
        /// Request adapter callback
        auto &renderer{*static_cast<webgpu_renderer*>(data)};
        auto &logger{renderer.logger};
        auto &webgpu{renderer.webgpu};
        if(message) logger << "WebGPU: Request adapter callback message: " << message;
        if(auto status{static_cast<wgpu::RequestAdapterStatus>(status_c)}; status != wgpu::RequestAdapterStatus::Success) {
          logger << "ERROR: WebGPU adapter request failure, status " << enum_wgpu_name<wgpu::RequestAdapterStatus>(status_c);
          throw std::runtime_error{"WebGPU: Could not get adapter"};
        }

        auto &adapter{webgpu.adapter};
        adapter = wgpu::Adapter::Acquire(adapter_ptr);
        if(!adapter) throw std::runtime_error{"WebGPU: Could not acquire adapter"};

        // report surface and adapter capabilities
        #ifndef NDEBUG
          {
            wgpu::SurfaceCapabilities surface_capabilities;
            webgpu.surface.GetCapabilities(adapter, &surface_capabilities);
            for(size_t i{0}; i != surface_capabilities.formatCount; ++i) {
              logger << "DEBUG: WebGPU surface capabilities: texture formats: " << magic_enum::enum_name(surface_capabilities.formats[i]);
            }
            for(size_t i{0}; i != surface_capabilities.presentModeCount; ++i) {
              logger << "DEBUG: WebGPU surface capabilities: present modes: " << magic_enum::enum_name(surface_capabilities.presentModes[i]);
            }
            for(size_t i{0}; i != surface_capabilities.alphaModeCount; ++i) {
              logger << "DEBUG: WebGPU surface capabilities: alpha modes: " << magic_enum::enum_name(surface_capabilities.alphaModes[i]);
            }
          }
        #endif // NDEBUG
        webgpu.surface_preferred_format = webgpu.surface.GetPreferredFormat(adapter);
        logger << "WebGPU surface preferred format for this adapter: " << magic_enum::enum_name(webgpu.surface_preferred_format);
        if(webgpu.surface_preferred_format == wgpu::TextureFormat::Undefined) {
          webgpu.surface_preferred_format = wgpu::TextureFormat::BGRA8Unorm;
          logger << "WebGPU manually specifying preferred format: " << magic_enum::enum_name(webgpu.surface_preferred_format);
        }

        {
          wgpu::AdapterInfo adapter_info;
          adapter.GetInfo(&adapter_info);
          #ifndef NDEBUG
            logger << "DEBUG: WebGPU adapter info: vendor: " << adapter_info.vendor;
            logger << "DEBUG: WebGPU adapter info: architecture: " << adapter_info.architecture;
            logger << "DEBUG: WebGPU adapter info: device: " << adapter_info.device;
            logger << "DEBUG: WebGPU adapter info: description: " << adapter_info.description;
            logger << "DEBUG: WebGPU adapter info: vendorID:deviceID: " << adapter_info.vendorID << ":" << adapter_info.deviceID;
            logger << "DEBUG: WebGPU adapter info: backendType: " << magic_enum::enum_name(adapter_info.backendType);
            logger << "DEBUG: WebGPU adapter info: adapterType: " << magic_enum::enum_name(adapter_info.adapterType);
          #endif // NDEBUG
          logger << "WebGPU adapter info: " << adapter_info.description << " (" << magic_enum::enum_name(adapter_info.backendType) << ", " << adapter_info.vendor << ", " << adapter_info.architecture << ")";
        }
        #ifndef NDEBUG
          {
            wgpu::AdapterProperties adapter_properties;
            adapter.GetProperties(&adapter_properties);
            // TODO: wgpuAdapterGetProperties is deprecated, use wgpuAdapterGetInfo instead - C++ wrapper needs to be updated
            logger << "DEBUG: WebGPU adapter properties: vendorID: " << adapter_properties.vendorID;
            logger << "DEBUG: WebGPU adapter properties: vendorName: " << adapter_properties.vendorName;
            logger << "DEBUG: WebGPU adapter properties: architecture: " << adapter_properties.architecture;
            logger << "DEBUG: WebGPU adapter properties: deviceID: " << adapter_properties.deviceID;
            logger << "DEBUG: WebGPU adapter properties: name: " << adapter_properties.name;
            logger << "DEBUG: WebGPU adapter properties: driverDescription: " << adapter_properties.driverDescription;
            logger << "DEBUG: WebGPU adapter properties: backendType: " << magic_enum::enum_name(adapter_properties.backendType);
            logger << "DEBUG: WebGPU adapter properties: adapterType: " << magic_enum::enum_name(adapter_properties.adapterType);
            logger << "DEBUG: WebGPU adapter properties: compatibilityMode: " << std::boolalpha << adapter_properties.compatibilityMode;
            logger << "DEBUG: WebGPU adapter properties: nextInChain: " << adapter_properties.nextInChain;
          }
        #endif // NDEBUG
        std::set<wgpu::FeatureName> adapter_features;
        {
          // see https://developer.mozilla.org/en-US/docs/Web/API/GPUSupportedFeatures and https://www.w3.org/TR/webgpu/#feature-index
          auto const count{adapter.EnumerateFeatures(nullptr)};
          logger << "DEBUG: WebGPU adapter features count: " << count;
          std::vector<wgpu::FeatureName> adapter_features_arr(count);
          adapter.EnumerateFeatures(adapter_features_arr.data());
          for(unsigned int i{0}; i != adapter_features_arr.size(); ++i) {
            adapter_features.emplace(adapter_features_arr[i]);
          }
        }
        for(auto const feature : adapter_features) {
          logger << "DEBUG: WebGPU adapter features: " << enum_wgpu_name<wgpu::FeatureName, WGPUFeatureName>(feature);
        }

        wgpu::SupportedLimits adapter_limits;
        bool const result{adapter.GetLimits(&adapter_limits)};
        if(!result) throw std::runtime_error{"WebGPU: Could not query adapter limits"};
        #ifndef NDEBUG
          logger << "DEBUG: WebGPU adapter limits result: " << std::boolalpha << result;
          logger << "DEBUG: WebGPU adapter limits nextInChain: " << adapter_limits.nextInChain;
          logger << "DEBUG: WebGPU adapter limits maxTextureDimension1D: " << adapter_limits.limits.maxTextureDimension1D;
          logger << "DEBUG: WebGPU adapter limits maxTextureDimension2D: " << adapter_limits.limits.maxTextureDimension2D;
          logger << "DEBUG: WebGPU adapter limits maxTextureDimension3D: " << adapter_limits.limits.maxTextureDimension3D;
          logger << "DEBUG: WebGPU adapter limits maxTextureArrayLayers: " << adapter_limits.limits.maxTextureArrayLayers;
          logger << "DEBUG: WebGPU adapter limits maxBindGroups: " << adapter_limits.limits.maxBindGroups;
          logger << "DEBUG: WebGPU adapter limits maxBindGroupsPlusVertexBuffers: " << adapter_limits.limits.maxBindGroupsPlusVertexBuffers;
          logger << "DEBUG: WebGPU adapter limits maxBindingsPerBindGroup: " << adapter_limits.limits.maxBindingsPerBindGroup;
          logger << "DEBUG: WebGPU adapter limits maxDynamicUniformBuffersPerPipelineLayout: " << adapter_limits.limits.maxDynamicUniformBuffersPerPipelineLayout;
          logger << "DEBUG: WebGPU adapter limits maxDynamicStorageBuffersPerPipelineLayout: " << adapter_limits.limits.maxDynamicStorageBuffersPerPipelineLayout;
          logger << "DEBUG: WebGPU adapter limits maxSamplersPerShaderStage: " << adapter_limits.limits.maxSamplersPerShaderStage;
          logger << "DEBUG: WebGPU adapter limits maxStorageBuffersPerShaderStage: " << adapter_limits.limits.maxStorageBuffersPerShaderStage;
          logger << "DEBUG: WebGPU adapter limits maxStorageTexturesPerShaderStage: " << adapter_limits.limits.maxStorageTexturesPerShaderStage;
          logger << "DEBUG: WebGPU adapter limits maxUniformBuffersPerShaderStage: " << adapter_limits.limits.maxUniformBuffersPerShaderStage;
          logger << "DEBUG: WebGPU adapter limits maxUniformBufferBindingSize: " << adapter_limits.limits.maxUniformBufferBindingSize;
          logger << "DEBUG: WebGPU adapter limits maxStorageBufferBindingSize: " << adapter_limits.limits.maxStorageBufferBindingSize;
          logger << "DEBUG: WebGPU adapter limits minUniformBufferOffsetAlignment: " << adapter_limits.limits.minUniformBufferOffsetAlignment;
          logger << "DEBUG: WebGPU adapter limits minStorageBufferOffsetAlignment: " << adapter_limits.limits.minStorageBufferOffsetAlignment;
          logger << "DEBUG: WebGPU adapter limits maxVertexBuffers: " << adapter_limits.limits.maxVertexBuffers;
          logger << "DEBUG: WebGPU adapter limits maxBufferSize: " << adapter_limits.limits.maxBufferSize;
          logger << "DEBUG: WebGPU adapter limits maxVertexAttributes: " << adapter_limits.limits.maxVertexAttributes;
          logger << "DEBUG: WebGPU adapter limits maxVertexBufferArrayStride: " << adapter_limits.limits.maxVertexBufferArrayStride;
          logger << "DEBUG: WebGPU adapter limits maxInterStageShaderComponents: " << adapter_limits.limits.maxInterStageShaderComponents;
          logger << "DEBUG: WebGPU adapter limits maxInterStageShaderVariables: " << adapter_limits.limits.maxInterStageShaderVariables;
          logger << "DEBUG: WebGPU adapter limits maxColorAttachments: " << adapter_limits.limits.maxColorAttachments;
          logger << "DEBUG: WebGPU adapter limits maxColorAttachmentBytesPerSample: " << adapter_limits.limits.maxColorAttachmentBytesPerSample;
          logger << "DEBUG: WebGPU adapter limits maxComputeWorkgroupStorageSize: " << adapter_limits.limits.maxComputeWorkgroupStorageSize;
          logger << "DEBUG: WebGPU adapter limits maxComputeInvocationsPerWorkgroup: " << adapter_limits.limits.maxComputeInvocationsPerWorkgroup;
          logger << "DEBUG: WebGPU adapter limits maxComputeWorkgroupSizeX: " << adapter_limits.limits.maxComputeWorkgroupSizeX;
          logger << "DEBUG: WebGPU adapter limits maxComputeWorkgroupSizeY: " << adapter_limits.limits.maxComputeWorkgroupSizeY;
          logger << "DEBUG: WebGPU adapter limits maxComputeWorkgroupSizeZ: " << adapter_limits.limits.maxComputeWorkgroupSizeZ;
          logger << "DEBUG: WebGPU adapter limits maxComputeWorkgroupsPerDimension: " << adapter_limits.limits.maxComputeWorkgroupsPerDimension;
        #endif // NDEBUG

        // specify required features for the device
        std::set<wgpu::FeatureName> required_features{
          wgpu::FeatureName::Depth32FloatStencil8,
          #ifndef NDEBUG
            wgpu::FeatureName::TimestampQuery,
          #endif // NDEBUG
          wgpu::FeatureName::TextureCompressionBC,
          wgpu::FeatureName::IndirectFirstInstance,
        };
        std::set<wgpu::FeatureName> desired_features{
          wgpu::FeatureName::ShaderF16,
          wgpu::FeatureName::Float32Filterable,
        };

        std::vector<wgpu::FeatureName> required_features_arr;
        for(auto const feature : required_features) {
          if(!adapter_features.contains(feature)) {
            logger << "WebGPU: Required adapter feature " << magic_enum::enum_name(feature) << " unavailable, cannot continue";
            throw std::runtime_error{"WebGPU: Required adapter feature " + std::string{magic_enum::enum_name(feature)} + " not available"};
          }
          logger << "WebGPU: Required adapter feature: " << magic_enum::enum_name(feature) << " requested";
          required_features_arr.emplace_back(feature);
        }
        for(auto const feature : desired_features) {
          if(!adapter_features.contains(feature)) {
            logger << "WebGPU: Desired adapter feature " << magic_enum::enum_name(feature) << " unavailable, continuing without it";
            continue;
          }
          logger << "WebGPU: Desired adapter feature " << magic_enum::enum_name(feature) << " requested";
          required_features_arr.emplace_back(feature);
        }

        // specify required limits for the device
        struct limit {
          wgpu::Limits required{
            .maxTextureDimension2D{3840},
            .maxTextureArrayLayers{1},
            .maxBindGroups{2},
            .maxUniformBuffersPerShaderStage{1},
            .maxUniformBufferBindingSize{sizeof(uniforms)},
            .maxVertexBuffers{1},
            .maxBufferSize{6 * 2 * sizeof(float)},
            .maxVertexAttributes{1},
            .maxVertexBufferArrayStride{2 * sizeof(float)},
          };
          wgpu::Limits desired{
            .maxTextureDimension2D{8192},
          };
        } requested_limits;

        auto require_limit{[&]<typename T>(std::string const &name, T available, T required, T desired){
          constexpr auto undefined{std::numeric_limits<T>::max()};
          if(required == undefined) {                                           // no hard requirement for this value
            if(desired == undefined) {                                          //   no specific desire for this value
              return undefined;                                                 //     we don't care about the value
            } else {                                                            //   we have a desire for a specific value
              if(available == undefined) {                                      //     but it's not available
                logger << "WebGPU: Desired minimum limit for " << name << " is " << desired << " but is unavailable, ignoring";
                return undefined;                                               //       that's fine, we don't care
              } else {                                                          //     some limit is available
                logger << "WebGPU: Desired minimum limit for " << name << " is " << desired << ", requesting " << std::min(desired, available);
                return std::min(desired, available);                            //       we'll accept our desired amount or the limit, whichever is lowest
              }
            }
          } else {                                                              // we have a hard requirement for this value
            if(available == undefined) {                                        //   but it's not available
              logger << "WebGPU: Required minimum limit " << required << " is not available for " << name << " (limit undefined), cannot continue";
              throw std::runtime_error("WebGPU: Required adapter limits not met (limit undefined)");
            } else {                                                            //   some limit is available
              if(available < required) {                                        //     but the limit is below our requirement
                logger << "WebGPU: Required minimum limit " << required << " is not available for " << name << " (max " << available << "), cannot continue";
                throw std::runtime_error("WebGPU: Required adapter limits not met");
              } else {                                                          //     the limit is acceptable
                if(desired == undefined) {                                      //       we have no desire beyond the basic requirement
                  logger << "WebGPU: Required minimum limit for " << name << " is " << required << ", available";
                  return required;                                              //         we'll accept the required minimum
                } else {                                                        //       we desire a value beyond the basic requirement
                  assert(desired > required);                                   //         make sure we're not requesting nonsense with desired values below required minimum
                  logger << "WebGPU: Desired minimum limit for " << name << " is " << desired << ", requesting " << std::min(desired, available);
                  return std::min(desired, available);                          //         we'll accept our desired amount or the limit, whichever is lowest
                }
              }
            }
          }
        }};

        wgpu::RequiredLimits const required_limits{
          .limits{                                                              // see https://www.w3.org/TR/webgpu/#limit-default
            #define REQUIRE_LIMIT(limit) .limit{require_limit(#limit, adapter_limits.limits.limit, requested_limits.required.limit, requested_limits.desired.limit)}
            REQUIRE_LIMIT(maxTextureDimension1D),
            REQUIRE_LIMIT(maxTextureDimension2D),
            REQUIRE_LIMIT(maxTextureDimension3D),
            REQUIRE_LIMIT(maxTextureArrayLayers),
            REQUIRE_LIMIT(maxBindGroups),
            REQUIRE_LIMIT(maxBindGroupsPlusVertexBuffers),
            REQUIRE_LIMIT(maxBindingsPerBindGroup),
            REQUIRE_LIMIT(maxDynamicUniformBuffersPerPipelineLayout),
            REQUIRE_LIMIT(maxDynamicStorageBuffersPerPipelineLayout),
            REQUIRE_LIMIT(maxSampledTexturesPerShaderStage),
            REQUIRE_LIMIT(maxSamplersPerShaderStage),
            REQUIRE_LIMIT(maxStorageBuffersPerShaderStage),
            REQUIRE_LIMIT(maxStorageTexturesPerShaderStage),
            REQUIRE_LIMIT(maxUniformBuffersPerShaderStage),
            REQUIRE_LIMIT(maxUniformBufferBindingSize),
            REQUIRE_LIMIT(maxStorageBufferBindingSize),
            REQUIRE_LIMIT(minUniformBufferOffsetAlignment),
            REQUIRE_LIMIT(minStorageBufferOffsetAlignment),
            // special treatment for minimum rather than maximum limits may be required, see notes for "alignment" at https://www.w3.org/TR/webgpu/#limit-default:
            //.minUniformBufferOffsetAlignment{adapter_limits.limits.minUniformBufferOffsetAlignment},
            //.minStorageBufferOffsetAlignment{adapter_limits.limits.minStorageBufferOffsetAlignment},
            REQUIRE_LIMIT(maxVertexBuffers),
            REQUIRE_LIMIT(maxBufferSize),
            REQUIRE_LIMIT(maxVertexAttributes),
            REQUIRE_LIMIT(maxVertexBufferArrayStride),
            REQUIRE_LIMIT(maxInterStageShaderComponents),
            REQUIRE_LIMIT(maxInterStageShaderVariables),
            REQUIRE_LIMIT(maxColorAttachments),
            REQUIRE_LIMIT(maxColorAttachmentBytesPerSample),
            REQUIRE_LIMIT(maxComputeWorkgroupStorageSize),
            REQUIRE_LIMIT(maxComputeInvocationsPerWorkgroup),
            REQUIRE_LIMIT(maxComputeWorkgroupSizeX),
            REQUIRE_LIMIT(maxComputeWorkgroupSizeY),
            REQUIRE_LIMIT(maxComputeWorkgroupSizeZ),
            REQUIRE_LIMIT(maxComputeWorkgroupsPerDimension),
            #undef REQUIRE_LIMIT
          },
        };

        // request a device
        wgpu::DeviceDescriptor device_descriptor{
          .requiredFeatureCount{required_features_arr.size()},
          .requiredFeatures{required_features_arr.data()},
          .requiredLimits{&required_limits},
          .defaultQueue{
            .label{"Default queue"},
          },
          .deviceLostCallback{[](WGPUDeviceLostReason reason_c, char const *message, void *data){
            /// Device lost callback
            auto &renderer{*static_cast<webgpu_renderer*>(data)};
            auto &logger{renderer.logger};
            logger << "ERROR: WebGPU lost device, reason " << enum_wgpu_name<wgpu::DeviceLostReason>(reason_c) << ": " << message;
          }},
          .deviceLostUserdata{&renderer},
        };

        adapter.RequestDevice(
          &device_descriptor,
          [](WGPURequestDeviceStatus status_c, WGPUDevice device_ptr,  const char *message,  void *data){
            /// Request device callback
            auto &renderer{*static_cast<webgpu_renderer*>(data)};
            auto &logger{renderer.logger};
            auto &webgpu{renderer.webgpu};
            if(message) logger << "WebGPU: Request device callback message: " << message;
            if(auto status{static_cast<wgpu::RequestDeviceStatus>(status_c)}; status != wgpu::RequestDeviceStatus::Success) {
              logger << "ERROR: WebGPU device request failure, status " << enum_wgpu_name<wgpu::RequestDeviceStatus>(status_c);
              throw std::runtime_error{"WebGPU: Could not get adapter"};
            }
            auto &device{webgpu.device};
            device = wgpu::Device::Acquire(device_ptr);

            // report device capabilities
            std::set<wgpu::FeatureName> device_features;
            {
              auto const count{device.EnumerateFeatures(nullptr)};
              #ifndef NDEBUG
                logger << "DEBUG: WebGPU device features count: " << count;
              #endif // NDEBUG
              std::vector<wgpu::FeatureName> device_features_arr(count);
              device.EnumerateFeatures(device_features_arr.data());
              for(unsigned int i{0}; i != device_features_arr.size(); ++i) {
                device_features.emplace(device_features_arr[i]);
              }
            }
            for(auto const feature : device_features) {
              logger << "DEBUG: WebGPU device features: " << magic_enum::enum_name(feature);
            }
            #ifndef NDEBUG
              {
                wgpu::SupportedLimits adapter_limits;
                bool result{device.GetLimits(&adapter_limits)};
                logger << "DEBUG: WebGPU device limits result: " << std::boolalpha << result;
                logger << "DEBUG: WebGPU device limits nextInChain: " << adapter_limits.nextInChain;
                logger << "DEBUG: WebGPU device limits maxTextureDimension1D: " << adapter_limits.limits.maxTextureDimension1D;
                logger << "DEBUG: WebGPU device limits maxTextureDimension2D: " << adapter_limits.limits.maxTextureDimension2D;
                logger << "DEBUG: WebGPU device limits maxTextureDimension3D: " << adapter_limits.limits.maxTextureDimension3D;
                logger << "DEBUG: WebGPU device limits maxTextureArrayLayers: " << adapter_limits.limits.maxTextureArrayLayers;
                logger << "DEBUG: WebGPU device limits maxBindGroups: " << adapter_limits.limits.maxBindGroups;
                logger << "DEBUG: WebGPU device limits maxBindGroupsPlusVertexBuffers: " << adapter_limits.limits.maxBindGroupsPlusVertexBuffers;
                logger << "DEBUG: WebGPU device limits maxBindingsPerBindGroup: " << adapter_limits.limits.maxBindingsPerBindGroup;
                logger << "DEBUG: WebGPU device limits maxDynamicUniformBuffersPerPipelineLayout: " << adapter_limits.limits.maxDynamicUniformBuffersPerPipelineLayout;
                logger << "DEBUG: WebGPU device limits maxDynamicStorageBuffersPerPipelineLayout: " << adapter_limits.limits.maxDynamicStorageBuffersPerPipelineLayout;
                logger << "DEBUG: WebGPU device limits maxSamplersPerShaderStage: " << adapter_limits.limits.maxSamplersPerShaderStage;
                logger << "DEBUG: WebGPU device limits maxStorageBuffersPerShaderStage: " << adapter_limits.limits.maxStorageBuffersPerShaderStage;
                logger << "DEBUG: WebGPU device limits maxStorageTexturesPerShaderStage: " << adapter_limits.limits.maxStorageTexturesPerShaderStage;
                logger << "DEBUG: WebGPU device limits maxUniformBuffersPerShaderStage: " << adapter_limits.limits.maxUniformBuffersPerShaderStage;
                logger << "DEBUG: WebGPU device limits maxUniformBufferBindingSize: " << adapter_limits.limits.maxUniformBufferBindingSize;
                logger << "DEBUG: WebGPU device limits maxStorageBufferBindingSize: " << adapter_limits.limits.maxStorageBufferBindingSize;
                logger << "DEBUG: WebGPU device limits minUniformBufferOffsetAlignment: " << adapter_limits.limits.minUniformBufferOffsetAlignment;
                logger << "DEBUG: WebGPU device limits minStorageBufferOffsetAlignment: " << adapter_limits.limits.minStorageBufferOffsetAlignment;
                logger << "DEBUG: WebGPU device limits maxVertexBuffers: " << adapter_limits.limits.maxVertexBuffers;
                logger << "DEBUG: WebGPU device limits maxBufferSize: " << adapter_limits.limits.maxBufferSize;
                logger << "DEBUG: WebGPU device limits maxVertexAttributes: " << adapter_limits.limits.maxVertexAttributes;
                logger << "DEBUG: WebGPU device limits maxVertexBufferArrayStride: " << adapter_limits.limits.maxVertexBufferArrayStride;
                logger << "DEBUG: WebGPU device limits maxInterStageShaderComponents: " << adapter_limits.limits.maxInterStageShaderComponents;
                logger << "DEBUG: WebGPU device limits maxInterStageShaderVariables: " << adapter_limits.limits.maxInterStageShaderVariables;
                logger << "DEBUG: WebGPU device limits maxColorAttachments: " << adapter_limits.limits.maxColorAttachments;
                logger << "DEBUG: WebGPU device limits maxColorAttachmentBytesPerSample: " << adapter_limits.limits.maxColorAttachmentBytesPerSample;
                logger << "DEBUG: WebGPU device limits maxComputeWorkgroupStorageSize: " << adapter_limits.limits.maxComputeWorkgroupStorageSize;
                logger << "DEBUG: WebGPU device limits maxComputeInvocationsPerWorkgroup: " << adapter_limits.limits.maxComputeInvocationsPerWorkgroup;
                logger << "DEBUG: WebGPU device limits maxComputeWorkgroupSizeX: " << adapter_limits.limits.maxComputeWorkgroupSizeX;
                logger << "DEBUG: WebGPU device limits maxComputeWorkgroupSizeY: " << adapter_limits.limits.maxComputeWorkgroupSizeY;
                logger << "DEBUG: WebGPU device limits maxComputeWorkgroupSizeZ: " << adapter_limits.limits.maxComputeWorkgroupSizeZ;
                logger << "DEBUG: WebGPU device limits maxComputeWorkgroupsPerDimension: " << adapter_limits.limits.maxComputeWorkgroupsPerDimension;
              }
            #endif // NDEBUG

            device.SetUncapturedErrorCallback(
              [](WGPUErrorType type, char const *message, void *data){
                /// Uncaptured error callback
                auto &renderer{*static_cast<webgpu_renderer*>(data)};
                auto &logger{renderer.logger};
                logger << "ERROR: WebGPU uncaptured error " << enum_wgpu_name<wgpu::ErrorType>(type) << ": " << message;
              },
              &renderer
            );
          },
          data
        );
      },
      this
    );
  }

  emscripten_set_main_loop_arg([](void *data){
    /// Dispatch the loop waiting for WebGPU to become ready
    auto &renderer{*static_cast<webgpu_renderer*>(data)};
    renderer.wait_to_configure_loop();
  }, this, 0, true);                                                            // loop function, user data, FPS (0 to use browser requestAnimationFrame mechanism), simulate infinite loop
  std::unreachable();
}

void webgpu_renderer::init_swapchain() {
  /// Create or recreate the swapchain for the current viewport size
  wgpu::SwapChainDescriptor swapchain_descriptor{
    .label{"Swapchain 1"},
    .usage{wgpu::TextureUsage::RenderAttachment},
    .format{webgpu.surface_preferred_format},
    .width{ window.viewport_size.x},
    .height{window.viewport_size.y},
    .presentMode{wgpu::PresentMode::Fifo},
  };
  webgpu.swapchain = webgpu.device.CreateSwapChain(webgpu.surface, &swapchain_descriptor);
}

void webgpu_renderer::init_depth_texture() {
  /// Create or recreate the depth buffer and its texture view
  {
    wgpu::TextureDescriptor depth_texture_descriptor{
      .label{"Depth texture 1"},
      .usage{wgpu::TextureUsage::RenderAttachment},
      .dimension{wgpu::TextureDimension::e2D},
      .size{
        window.viewport_size.x,
        window.viewport_size.y,
        1
      },
      .format{webgpu.depth_texture_format},
      .viewFormatCount{1},
      .viewFormats{&webgpu.depth_texture_format},
    };
    webgpu.depth_texture = webgpu.device.CreateTexture(&depth_texture_descriptor);
  }
  {
    wgpu::TextureViewDescriptor depth_texture_view_descriptor{
      .label{"Depth texture view 1"},
      .format{webgpu.depth_texture_format},
      .dimension{wgpu::TextureViewDimension::e2D},
      .mipLevelCount{1},
      .arrayLayerCount{1},
      .aspect{wgpu::TextureAspect::DepthOnly},
    };
    webgpu.depth_texture_view = webgpu.depth_texture.CreateView(&depth_texture_view_descriptor);
  }
}

void webgpu_renderer::wait_to_configure_loop() {
  /// Check if initialisation has completed and the WebGPU system is ready for configuration
  /// Since init occurs asynchronously, some emscripten ticks are needed before this becomes true
  if(!webgpu.device) {
    logger << "WebGPU: Waiting for device to become available";
    // TODO: sensible timeout
    return;
  }
  emscripten_cancel_main_loop();

  configure();

  if(postinit_callback) {
    logger << "WebGPU: Configuration complete, running post-init tasks";
    postinit_callback(webgpu);                                                  // perform any user-provided post-init tasks before launching the main loop
  }

  logger << "WebGPU: Launching main loop";
  emscripten_set_main_loop_arg([](void *data){
    /// Main pseudo-loop waiting for initialisation to complete
    auto &renderer{*static_cast<webgpu_renderer*>(data)};
    renderer.main_loop_callback();
  }, this, 0, true);                                                            // loop function, user data, FPS (0 to use browser requestAnimationFrame mechanism), simulate infinite loop
  std::unreachable();
}

void webgpu_renderer::configure() {
  /// When the device is ready, configure the WebGPU system
  logger << "WebGPU device ready, configuring surface";
  {
    wgpu::SurfaceConfiguration surface_configuration{
      .device{webgpu.device},
      .format{webgpu.surface_preferred_format},
      .viewFormats{nullptr},
      .width{ window.viewport_size.x},
      .height{window.viewport_size.y},
    };
    webgpu.surface.Configure(&surface_configuration);
  }

  logger << "WebGPU creating swapchain";
  init_swapchain();

  logger << "WebGPU acquiring queue";
  webgpu.queue = webgpu.device.GetQueue();

  logger << "WebGPU assembling shaders";
  {
    wgpu::ShaderModuleWGSLDescriptor shader_module_wgsl_decriptor;
    shader_module_wgsl_decriptor.code = render::shaders::default_wgsl;
    wgpu::ShaderModuleDescriptor shader_module_descriptor{
      .nextInChain{&shader_module_wgsl_decriptor},
      .label{"Shader module 1"},
    };
    wgpu::ShaderModule shader_module{webgpu.device.CreateShaderModule(&shader_module_descriptor)};

    logger << "WebGPU configuring pipeline";

    std::array vertex_attributes{
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x3},
        .offset{offsetof(vertex, position)},
        .shaderLocation{0},
      },
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x3},
        .offset{offsetof(vertex, normal)},
        .shaderLocation{1},
      },
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x4},
        .offset{offsetof(vertex, colour)},
        .shaderLocation{2},
      },
    };
    std::array instance_attributes{
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x4},                                 // we want a mat4, so we pass four vec4 rows per instance - see https://www.reddit.com/r/vulkan/comments/8zx1hn/comment/e2m2diq/
        .offset{offsetof(instance, model) + (sizeof(float) * 4 * 0)},
        .shaderLocation{3},
      },
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x4},
        .offset{offsetof(instance, model) + (sizeof(float) * 4 * 1)},
        .shaderLocation{4},
      },
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x4},
        .offset{offsetof(instance, model) + (sizeof(float) * 4 * 2)},
        .shaderLocation{5},
      },
      wgpu::VertexAttribute{
        .format{wgpu::VertexFormat::Float32x4},
        .offset{offsetof(instance, model) + (sizeof(float) * 4 * 3)},
        .shaderLocation{6},
      },
    };
    std::vector<wgpu::VertexBufferLayout> vertex_buffer_layouts{
      {
        .arrayStride{sizeof(vertex)},
        .attributeCount{vertex_attributes.size()},
        .attributes{vertex_attributes.data()},
      },
      {
        .arrayStride{sizeof(instance)},
        .stepMode{wgpu::VertexStepMode::Instance},                              // per-instance buffer
        .attributeCount{instance_attributes.size()},
        .attributes{instance_attributes.data()},
      },
    };

    wgpu::BlendState blend_state{
      .color{                                                                   // BlendComponent
        .operation{wgpu::BlendOperation::Add},                                  // initial values from https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
        .srcFactor{wgpu::BlendFactor::SrcAlpha},
        .dstFactor{wgpu::BlendFactor::OneMinusSrcAlpha},
      },
      .alpha{                                                                   // BlendComponent
        .operation{wgpu::BlendOperation::Add},                                  // these differ from defaults
        .srcFactor{wgpu::BlendFactor::Zero},
        .dstFactor{wgpu::BlendFactor::One},
        // TODO: compare with defaults
      },
    };
    wgpu::ColorTargetState colour_target_state{
      .format{webgpu.surface_preferred_format},
      .blend{&blend_state},
    };
    wgpu::FragmentState fragment_state{
      .module{shader_module},
      .entryPoint{"fs_main"},
      .constantCount{0},
      .constants{nullptr},
      .targetCount{1},
      .targets{&colour_target_state},
    };

    wgpu::DepthStencilState depth_stencil_state{
      .format{wgpu::TextureFormat::Depth24Plus},
      .depthWriteEnabled{true},
      .depthCompare{wgpu::CompareFunction::Less},
      .stencilFront{},                                                          // StencilFaceState
      .stencilBack{},                                                           // StencilFaceState
      .stencilReadMask{0},
      .stencilWriteMask{0},
      // TODO: tweak depth bias settings
    };

    wgpu::BindGroupLayoutEntry binding_layout{
      .binding{0},                                                              // binding index as used in the @binding attribute in the shader
      .visibility{wgpu::ShaderStage::Vertex},
      .buffer{                                                                  // BufferBindingLayout
        .type{wgpu::BufferBindingType::Uniform},
        .minBindingSize{sizeof(uniforms)},
      },
      .sampler{},                                                               // SamplerBindingLayout
      .texture{},                                                               // TextureBindingLayout
      .storageTexture{},                                                        // StorageTextureBindingLayout
    };
    wgpu::BindGroupLayoutDescriptor bind_group_layout_descriptor{
      .label{"Bind group layout 1"},
      .entryCount{1},
      .entries{&binding_layout},
    };
    webgpu.bind_group_layout = webgpu.device.CreateBindGroupLayout(&bind_group_layout_descriptor);

    wgpu::PipelineLayoutDescriptor pipeline_layout_descriptor{
      .label{"Pipeline layout 1"},
      .bindGroupLayoutCount{1},
      .bindGroupLayouts{&webgpu.bind_group_layout},
    };
    wgpu::PipelineLayout pipeline_layout{webgpu.device.CreatePipelineLayout(&pipeline_layout_descriptor)};

    wgpu::RenderPipelineDescriptor render_pipeline_descriptor{
      .label{"Render pipeline 1"},
      .layout{std::move(pipeline_layout)},
      .vertex{                                                                  // VertexState
        .module{shader_module},
        .entryPoint{"vs_main"},
        .constantCount{0},
        .constants{nullptr},
        .bufferCount{vertex_buffer_layouts.size()},
        .buffers{vertex_buffer_layouts.data()},
      },
      .primitive{                                                               // PrimitiveState
        .cullMode{wgpu::CullMode::Back},
      },
      .depthStencil{&depth_stencil_state},
      .multisample{},
      .fragment{&fragment_state},
    };
    webgpu.pipeline = webgpu.device.CreateRenderPipeline(&render_pipeline_descriptor);
  }

  logger << "WebGPU creating depth texture";
  init_depth_texture();

  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false,   // target, userdata, use_capture, callback
    ([](int /*event_type*/, EmscriptenUiEvent const *event, void *data) {       // event_type == EMSCRIPTEN_EVENT_RESIZE
      auto &renderer{*static_cast<webgpu_renderer*>(data)};
      renderer.window.viewport_size.x = static_cast<unsigned int>(event->windowInnerWidth);
      renderer.window.viewport_size.y = static_cast<unsigned int>(event->windowInnerHeight);

      renderer.init_swapchain();
      renderer.init_depth_texture();
      return true;                                                              // the event was consumed
    })
  );

  build_scene();
}

void webgpu_renderer::build_scene() {
  /// Assemble the unchanging portions of the scene
  // set up test buffers

  // create buffers
  {
    // vertex buffer
    wgpu::BufferDescriptor vertex_buffer_descriptor{
      .label{"Vertex buffer 1"},
      .usage{wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex},
      .size{vertex_data.size() * sizeof(vertex_data[0])},
    };
    vertex_buffer = webgpu.device.CreateBuffer(&vertex_buffer_descriptor);
  }
  {
    // instance buffer (per-instance vertex buffer)
    wgpu::BufferDescriptor instance_buffer_descriptor{
      .label{"Instance buffer 1"},
      .usage{wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex},
      .size{num_instances * sizeof(instance)},
    };
    instance_buffer = webgpu.device.CreateBuffer(&instance_buffer_descriptor);
  }
  {
    // index buffer
    wgpu::BufferDescriptor index_buffer_descriptor{
      .label{"Index buffer 1"},
      .usage{wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index},
      .size{index_data.size() * sizeof(index_data[0])},
    };
    index_buffer = webgpu.device.CreateBuffer(&index_buffer_descriptor);
  }
  {
    // uniform buffer
    wgpu::BufferDescriptor uniform_buffer_desecriptor{
      .label{"Uniform buffer 1"},
      .usage{wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform},
      .size{sizeof(uniform_data)},
    };
    uniform_buffer = webgpu.device.CreateBuffer(&uniform_buffer_desecriptor);
  }

  // indirect draw command buffer
  {
    wgpu::BufferDescriptor indirect_buffer_descriptor{
      .label{"Indirect buffer 1"},
      .usage{wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Indirect},
      .size{sizeof(indirect_data)},
    };
    indirect_buffer = webgpu.device.CreateBuffer(&indirect_buffer_descriptor);
  }

  // uniform bind group
  wgpu::BindGroupEntry bind_group_entry{
    .binding{0},
    .buffer{uniform_buffer},
    .size{sizeof(uniforms)},
  };
  wgpu::BindGroupDescriptor bind_group_descriptor{
    .label{"Bind group 1"},
    .layout{webgpu.bind_group_layout},
    .entryCount{1},                                                             // must correspond to layout
    .entries{&bind_group_entry},
  };
  wgpu::BindGroup bind_group{webgpu.device.CreateBindGroup(&bind_group_descriptor)};

  // set up render bundle
  {
    wgpu::RenderBundleEncoderDescriptor render_bundle_encoder_descriptor{
      .label{"Render bundle encoder 1"},
      .colorFormatCount{1},
      .colorFormats{&webgpu.surface_preferred_format},
      .depthStencilFormat{webgpu.depth_texture_format},
    };
    wgpu::RenderBundleEncoder render_bundle_encoder{webgpu.device.CreateRenderBundleEncoder(&render_bundle_encoder_descriptor)};

    render_bundle_encoder.SetPipeline(webgpu.pipeline);                         // select which render pipeline to use
    render_bundle_encoder.SetVertexBuffer(0, vertex_buffer, 0, vertex_buffer.GetSize()); // slot, buffer, offset, size
    render_bundle_encoder.SetVertexBuffer(1, instance_buffer, 0, instance_buffer.GetSize()); // slot, buffer, offset, size
    render_bundle_encoder.SetIndexBuffer(index_buffer, wgpu::IndexFormat::Uint16, 0, index_buffer.GetSize()); // buffer, format, offset, size
    render_bundle_encoder.SetBindGroup(0, bind_group);                          // groupIndex, group, dynamicOffsetCount = 0, dynamicOffsets = nullptr

    render_bundle_encoder.DrawIndexedIndirect(indirect_buffer, 0);              // indirectBuffer, indirectOffset

    wgpu::RenderBundleDescriptor render_bundle_descriptor{
      .label{"Render bundle 1"},
    };
    render_bundles.emplace_back(render_bundle_encoder.Finish(&render_bundle_descriptor));
  }

  // populate buffer contents
  webgpu.queue.WriteBuffer(
    vertex_buffer,                                                              // buffer
    0,                                                                          // offset
    vertex_data.data(),                                                         // data
    vertex_data.size() * sizeof(vertex_data[0])                                 // size
  );

  webgpu.queue.WriteBuffer(
    index_buffer,                                                               // buffer
    0,                                                                          // offset
    index_data.data(),                                                          // data
    index_data.size() * sizeof(index_data[0])                                   // size
  );

  // indirect draw command data
  indirect_data = {
    .index_count{index_data.size() * decltype(index_data)::value_type::size()},
    .instance_count{num_instances},
  };

  webgpu.queue.WriteBuffer(
    indirect_buffer,                                                            // buffer
    0,                                                                          // offset
    &indirect_data,                                                             // data
    sizeof(indirect_data)                                                       // size
  );
}

void webgpu_renderer::draw(vec2f const& rotation) {
  /// Draw a frame
  {
    wgpu::CommandEncoderDescriptor command_encoder_descriptor{
      .label = "Command encoder 1"
    };
    wgpu::CommandEncoder command_encoder{webgpu.device.CreateCommandEncoder(&command_encoder_descriptor)};

    {
      // set up matrices
      static vec2f angles;
      angles += rotation;
      angles.x += 0.01f;                                                        // constant slow spin
      quatf model_rotation{quatf::from_euler_angles_rad(0.0, angles.x, 0.0)};

      vec3f camera_pos{0.0f, 10.0f, -25.0f};
      camera_pos.rotate_rad_x(angles.y);

      mat4f projection{make_projection_matrix(static_cast<vec2f>(window.viewport_size))};
      // TODO: reversed Z matrix adaptations: https://webgpu.github.io/webgpu-samples/?sample=reversedZ

      mat4f look_at{mat4f::create_look_at(
        camera_pos,                                                             // eye pos
        {0.0f, 0.0f, 0.0f},                                                     // target pos
        {0.0f, 1.0f, 0.0f}                                                      // up dir
      )};

      uniform_data.view_projection = projection * look_at;
      uniform_data.normal = mat3fwgpu{model_rotation.rotmatrix()};

      // per-instance data
      std::vector<instance> instance_data;
      instance_data.reserve(num_instances);
      for(unsigned int y = 0; y != grid_size.y; ++y) {
        for(unsigned int z = 0; z != grid_size.z; ++z) {
          for(unsigned int x = 0; x != grid_size.x; ++x) {
            mat4f const offset{mat4f::create_translation(((vec3f{vec3ui{x, y, z}} - (vec3f{grid_size} * 0.5f)) * 3.0f) + vec3f{0.0f, 0.0f, 70.0f})};
            instance_data.emplace_back(offset * model_rotation.transform());
          }
        }
      }

      // update buffer contents
      webgpu.queue.WriteBuffer(
        instance_buffer,                                                        // buffer
        0,                                                                      // offset
        instance_data.data(),                                                   // data
        instance_data.size() * sizeof(instance_data[0])                         // size
      );

      webgpu.queue.WriteBuffer(
        uniform_buffer,                                                         // buffer
        0,                                                                      // offset
        &uniform_data,                                                          // data
        sizeof(uniform_data)                                                    // size
      );
    }
    {
      // set up render pass
      command_encoder.PushDebugGroup("Render pass group 1");
      wgpu::TextureView texture_view{webgpu.swapchain.GetCurrentTextureView()};
      if(!texture_view) throw std::runtime_error{"Could not get current texture view from swap chain"};

      wgpu::RenderPassColorAttachment render_pass_colour_attachment{
        .view{texture_view},
        .loadOp{wgpu::LoadOp::Clear},
        .storeOp{wgpu::StoreOp::Store},
        .clearValue{wgpu::Color{0, 0.5, 0.5, 1.0}},
      };

      wgpu::RenderPassDepthStencilAttachment render_pass_depth_stencil_attachment{
        .view{webgpu.depth_texture_view},
        .depthLoadOp{wgpu::LoadOp::Clear},
        .depthStoreOp{wgpu::StoreOp::Store},
        .depthClearValue{1.0f},
      };
      render_pass_descriptor = {
        .label{"Render pass 1"},
        .colorAttachmentCount{1},
        .colorAttachments{&render_pass_colour_attachment},
        .depthStencilAttachment{&render_pass_depth_stencil_attachment},
      };

      wgpu::RenderPassEncoder render_pass_encoder{command_encoder.BeginRenderPass(&render_pass_descriptor)};

      render_pass_encoder.ExecuteBundles(render_bundles.size(), render_bundles.data());

      ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), render_pass_encoder.Get()); // render the outstanding GUI draw data

      // TODO: add timestamp query: https://eliemichel.github.io/LearnWebGPU/advanced-techniques/benchmarking/time.html / https://webgpu.github.io/webgpu-samples/?sample=timestampQuery

      // TODO: occlusion queries https://webgpu.github.io/webgpu-samples/?sample=occlusionQuery#main.ts

      // TODO: wireframe rendering pipeline per barycentric coordinates https://webgpu.github.io/webgpu-samples/?sample=wireframe#main.ts
      // TODO: render bundle culling https://github.com/toji/webgpu-bundle-culling

      render_pass_encoder.End();
      command_encoder.PopDebugGroup();
    }

    command_encoder.InsertDebugMarker("Debug marker 1");

    wgpu::CommandBufferDescriptor command_buffer_descriptor {
      .label = "Command buffer 1"
    };
    wgpu::CommandBuffer command_buffer{command_encoder.Finish(&command_buffer_descriptor)};

    //webgpu.queue.OnSubmittedWorkDone(
    //  [](WGPUQueueWorkDoneStatus status_c, void *data){
    //    /// Submitted work done callback - note, this only fires for the subsequent submit
    //    auto &renderer{*static_cast<webgpu_renderer*>(data)};
    //    auto &logger{renderer.logger};
    //    if(auto const status{static_cast<wgpu::QueueWorkDoneStatus>(status_c)}; status != wgpu::QueueWorkDoneStatus::Success) {
    //      logger << "ERROR: WebGPU queue submitted work failure, status: " << enum_wgpu_name<wgpu::QueueWorkDoneStatus>(status_c);
    //    }
    //    logger << "DEBUG: WebGPU queue submitted work done";
    //  },
    //  this
    //);

    webgpu.queue.Submit(1, &command_buffer);
  }
}

}
