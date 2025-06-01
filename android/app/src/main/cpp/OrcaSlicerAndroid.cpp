#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

// OrcaSlicer核心头文件
#include "libslic3r/libslic3r.h"
#include "libslic3r/Model.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/GCode/PreviewData.hpp"
#include "libslic3r/Format/STL.hpp"
#include "libslic3r/Format/AMF.hpp"
#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/Format/OBJ.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/AppConfig.hpp"

#define LOG_TAG "OrcaSlicerAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 前向声明，在OrcaSlicerJNI.cpp中实现
extern "C" void orca_slicer_notify_slicing_finished(bool success);

// 使用JSON库简化数据传输
using json = nlohmann::json;

// OrcaSlicer应用类
class OrcaSlicerApp {
public:
    OrcaSlicerApp(const std::string& data_path);
    ~OrcaSlicerApp();
    
    void setSurface(ANativeWindow* window, int width, int height);
    void surfaceDestroyed();
    void resume();
    void pause();
    
    bool importModel(const std::string& model_path);
    bool startSlicing();
    
    // 新增功能
    std::string getPrintersList();
    std::string getMaterialsList();
    std::string getPrintSettingsList();
    bool selectPrinter(const std::string& printer_name);
    bool selectMaterial(const std::string& material_name);
    bool selectPrintSettings(const std::string& print_settings_name);
    std::string getModelInfo();
    bool rotateModel(int object_id, float angle, int axis);
    bool scaleModel(int object_id, float scale);
    bool translateModel(int object_id, float x, float y, float z);
    bool deleteModel(int object_id);
    std::string getSlicePreviewInfo();
    std::string getGCodePreviewInfo();
    bool exportGCodeToFile(const std::string& file_path);
    
private:
    void initializeCore();
    void renderLoop();
    void setupGL();
    void render();
    void processSlicing();
    
    // 核心组件
    std::unique_ptr<Slic3r::Model> model;
    std::unique_ptr<Slic3r::Print> print;
    std::unique_ptr<Slic3r::DynamicPrintConfig> config;
    std::unique_ptr<Slic3r::PresetBundle> preset_bundle;
    std::unique_ptr<Slic3r::AppConfig> app_config;
    
    // 渲染相关
    ANativeWindow* window;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int width;
    int height;
    
    // 线程控制
    std::thread render_thread;
    std::mutex mutex;
    bool running;
    bool slicing_in_progress;
    std::string data_path;
};

OrcaSlicerApp::OrcaSlicerApp(const std::string& data_path)
    : window(nullptr), display(EGL_NO_DISPLAY), surface(EGL_NO_SURFACE), context(EGL_NO_CONTEXT),
      width(0), height(0), running(false), slicing_in_progress(false), data_path(data_path) {
    LOGI("OrcaSlicerApp constructor called with data path: %s", data_path.c_str());
    
    // 初始化核心组件
    initializeCore();
}

OrcaSlicerApp::~OrcaSlicerApp() {
    LOGI("OrcaSlicerApp destructor called");
    
    // 确保渲染线程已停止
    running = false;
    if (render_thread.joinable()) {
        render_thread.join();
    }
    
    // 清理EGL资源
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        eglTerminate(display);
    }
    
    // 释放窗口
    if (window) {
        ANativeWindow_release(window);
    }
}

void OrcaSlicerApp::initializeCore() {
    LOGI("Initializing OrcaSlicer core components");
    
    try {
        // 初始化配置
        app_config = std::make_unique<Slic3r::AppConfig>();
        app_config->set("datadir", data_path);
        
        // 初始化预设包
        preset_bundle = std::make_unique<Slic3r::PresetBundle>();
        preset_bundle->load_configbundle(data_path + "/orca-profiles.ini", app_config.get());
        
        // 创建模型和打印对象
        model = std::make_unique<Slic3r::Model>();
        print = std::make_unique<Slic3r::Print>();
        config = std::make_unique<Slic3r::DynamicPrintConfig>();
        
        // 加载默认配置
        *config = preset_bundle->full_config();
        print->apply(*config);
        
        LOGI("OrcaSlicer core components initialized successfully");
    } catch (const std::exception& e) {
        LOGE("Failed to initialize OrcaSlicer core: %s", e.what());
    }
}

void OrcaSlicerApp::setSurface(ANativeWindow* new_window, int new_width, int new_height) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 如果已经有窗口，先释放它
    if (window && window != new_window) {
        ANativeWindow_release(window);
    }
    
    window = new_window;
    
    // 如果提供了新的尺寸，更新它
    if (new_width > 0 && new_height > 0) {
        width = new_width;
        height = new_height;
    } else if (window) {
        // 否则从窗口获取尺寸
        width = ANativeWindow_getWidth(window);
        height = ANativeWindow_getHeight(window);
    }
    
    LOGI("Surface set with size: %dx%d", width, height);
    
    // 如果渲染线程未运行，启动它
    if (!running && window) {
        running = true;
        render_thread = std::thread(&OrcaSlicerApp::renderLoop, this);
    }
}

void OrcaSlicerApp::surfaceDestroyed() {
    std::lock_guard<std::mutex> lock(mutex);
    
    LOGI("Surface destroyed");
    
    // 停止渲染线程
    running = false;
    if (render_thread.joinable()) {
        render_thread.join();
    }
    
    // 清理EGL资源
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
            context = EGL_NO_CONTEXT;
        }
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
            surface = EGL_NO_SURFACE;
        }
        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }
    
    // 释放窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

void OrcaSlicerApp::resume() {
    std::lock_guard<std::mutex> lock(mutex);
    
    LOGI("Resuming OrcaSlicer");
    
    // 如果有窗口但渲染线程未运行，启动它
    if (window && !running) {
        running = true;
        render_thread = std::thread(&OrcaSlicerApp::renderLoop, this);
    }
}

void OrcaSlicerApp::pause() {
    std::lock_guard<std::mutex> lock(mutex);
    
    LOGI("Pausing OrcaSlicer");
    
    // 停止渲染线程
    running = false;
    if (render_thread.joinable()) {
        render_thread.join();
    }
}

bool OrcaSlicerApp::importModel(const std::string& model_path) {
    LOGI("Importing model from: %s", model_path.c_str());
    
    try {
        // 根据文件扩展名选择适当的导入器
        std::string extension = model_path.substr(model_path.find_last_of(".") + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "stl") {
            Slic3r::Model new_model = Slic3r::Model::read_from_file(model_path, Slic3r::LoadStrategy::LoadModel);
            *model = std::move(new_model);
        } else if (extension == "obj") {
            Slic3r::Model new_model = Slic3r::Model::read_from_file(model_path, Slic3r::LoadStrategy::LoadModel);
            *model = std::move(new_model);
        } else if (extension == "3mf") {
            Slic3r::Model new_model = Slic3r::Model::read_from_file(model_path, Slic3r::LoadStrategy::LoadModel);
            *model = std::move(new_model);
        } else if (extension == "amf") {
            Slic3r::Model new_model = Slic3r::Model::read_from_file(model_path, Slic3r::LoadStrategy::LoadModel);
            *model = std::move(new_model);
        } else {
            LOGE("Unsupported file format: %s", extension.c_str());
            return false;
        }
        
        // 准备模型用于打印
        model->arrange_objects(print->config());
        model->center_instances_around_point(Slic3r::Vec2d(0, 0));
        
        LOGI("Model imported successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to import model: %s", e.what());
        return false;
    }
}

bool OrcaSlicerApp::startSlicing() {
    LOGI("Starting slicing process");
    
    if (slicing_in_progress) {
        LOGE("Slicing already in progress");
        return false;
    }
    
    if (model->objects.empty()) {
        LOGE("No model loaded for slicing");
        return false;
    }
    
    // 在单独的线程中进行切片，以避免阻塞UI
    slicing_in_progress = true;
    std::thread slicing_thread(&OrcaSlicerApp::processSlicing, this);
    slicing_thread.detach();
    
    return true;
}

void OrcaSlicerApp::processSlicing() {
    LOGI("Processing slicing in background thread");
    
    bool success = false;
    
    try {
        // 准备打印对象
        print->clear();
        
        // 将模型添加到打印对象
        for (const auto& object : model->objects) {
            print->add_model_object(*object);
        }
        
        // 应用配置
        print->apply(*config);
        
        // 执行切片
        print->process();
        
        // 生成G代码（可选，取决于需求）
        // std::string gcode = print->export_gcode();
        
        success = true;
        LOGI("Slicing completed successfully");
    } catch (const std::exception& e) {
        LOGE("Slicing failed: %s", e.what());
    }
    
    // 切片完成，通知Java层
    slicing_in_progress = false;
    orca_slicer_notify_slicing_finished(success);
}

void OrcaSlicerApp::setupGL() {
    LOGI("Setting up OpenGL ES");
    
    // 获取默认显示
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return;
    }
    
    // 初始化EGL
    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        return;
    }
    
    LOGI("EGL initialized with version %d.%d", major, minor);
    
    // 配置EGL
    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs) || num_configs <= 0) {
        LOGE("Failed to choose EGL config");
        return;
    }
    
    // 创建EGL表面
    surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return;
    }
    
    // 创建EGL上下文
    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return;
    }
    
    // 使当前上下文成为活动上下文
    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("Failed to make EGL context current");
        return;
    }
    
    // 设置视口
    glViewport(0, 0, width, height);
    
    // 设置清除颜色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    LOGI("OpenGL ES setup completed");
}

void OrcaSlicerApp::render() {
    // 清除颜色和深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 在这里添加渲染代码
    // 例如，渲染模型、切片预览等
    
    // 交换缓冲区
    eglSwapBuffers(display, surface);
}

void OrcaSlicerApp::renderLoop() {
    LOGI("Render loop started");
    
    // 设置OpenGL
    setupGL();
    
    // 渲染循环
    while (running) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE && context != EGL_NO_CONTEXT) {
            render();
        }
        
        // 限制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    LOGI("Render loop ended");
}

// 新增功能实现
std::string OrcaSlicerApp::getPrintersList() {
    LOGI("Getting printers list");
    
    try {
        json printers_json = json::array();
        
        // 获取打印机预设
        for (const auto& preset : preset_bundle->printers) {
            if (!preset.is_default && !preset.is_system) {
                json printer = {
                    {"name", preset.name},
                    {"vendor", preset.vendor ? preset.vendor->name : ""},
                    {"technology", Slic3r::static_print_technology_type_name(preset.printer_technology())}
                };
                printers_json.push_back(printer);
            }
        }
        
        return printers_json.dump();
    } catch (const std::exception& e) {
        LOGE("Failed to get printers list: %s", e.what());
        return "[]"; // 返回空数组
    }
}

std::string OrcaSlicerApp::getMaterialsList() {
    LOGI("Getting materials list");
    
    try {
        json materials_json = json::array();
        
        // 获取材料预设
        for (const auto& preset : preset_bundle->filaments) {
            if (!preset.is_default && !preset.is_system) {
                json material = {
                    {"name", preset.name},
                    {"vendor", preset.vendor ? preset.vendor->name : ""},
                    {"filament_type", preset.config.opt_string("filament_type", 0)}
                };
                materials_json.push_back(material);
            }
        }
        
        return materials_json.dump();
    } catch (const std::exception& e) {
        LOGE("Failed to get materials list: %s", e.what());
        return "[]"; // 返回空数组
    }
}

std::string OrcaSlicerApp::getPrintSettingsList() {
    LOGI("Getting print settings list");
    
    try {
        json settings_json = json::array();
        
        // 获取打印设置预设
        for (const auto& preset : preset_bundle->prints) {
            if (!preset.is_default && !preset.is_system) {
                json setting = {
                    {"name", preset.name},
                    {"vendor", preset.vendor ? preset.vendor->name : ""},
                    {"layer_height", preset.config.opt_float("layer_height")}
                };
                settings_json.push_back(setting);
            }
        }
        
        return settings_json.dump();
    } catch (const std::exception& e) {
        LOGE("Failed to get print settings list: %s", e.what());
        return "[]"; // 返回空数组
    }
}

bool OrcaSlicerApp::selectPrinter(const std::string& printer_name) {
    LOGI("Selecting printer: %s", printer_name.c_str());
    
    try {
        // 查找并选择打印机预设
        for (const auto& preset : preset_bundle->printers) {
            if (preset.name == printer_name) {
                preset_bundle->set_printer_preset(preset_bundle->printers.find_preset_index(printer_name));
                *config = preset_bundle->full_config();
                print->apply(*config);
                LOGI("Printer selected successfully");
                return true;
            }
        }
        
        LOGE("Printer not found: %s", printer_name.c_str());
        return false;
    } catch (const std::exception& e) {
        LOGE("Failed to select printer: %s", e.what());
        return false;
    }
}

bool OrcaSlicerApp::selectMaterial(const std::string& material_name) {
    LOGI("Selecting material: %s", material_name.c_str());
    
    try {
        // 查找并选择材料预设
        for (const auto& preset : preset_bundle->filaments) {
            if (preset.name == material_name) {
                preset_bundle->set_filament_preset(0, preset_bundle->filaments.find_preset_index(material_name));
                *config = preset_bundle->full_config();
                print->apply(*config);
                LOGI("Material selected successfully");
                return true;
            }
        }
        
        LOGE("Material not found: %s", material_name.c_str());
        return false;
    } catch (const std::exception& e) {
        LOGE("Failed to select material: %s", e.what());
        return false;
    }
}

bool OrcaSlicerApp::selectPrintSettings(const std::string& print_settings_name) {
    LOGI("Selecting print settings: %s", print_settings_name.c_str());
    
    try {
        // 查找并选择打印设置预设
        for (const auto& preset : preset_bundle->prints) {
            if (preset.name == print_settings_name) {
                preset_bundle->set_print_preset(preset_bundle->prints.find_preset_index(print_settings_name));
                *config = preset_bundle->full_config();
                print->apply(*config);
                LOGI("Print settings selected successfully");
                return true;
            }
        }
        
        LOGE("Print settings not found: %s", print_settings_name.c_str());
        return false;
    } catch (const std::exception& e) {
        LOGE("Failed to select print settings: %s", e.what());
        return false;
    }
}

std::string OrcaSlicerApp::getModelInfo() {
    LOGI("Getting model information");
    
    try {
        json model_json = json::array();
        
        // 获取模型信息
        for (size_t i = 0; i < model->objects.size(); i++) {
            const auto& object = model->objects[i];
            
            json obj = {
                {"id", i},
                {"name", object->name},
                {"volume_count", object->volumes.size()},
                {"instance_count", object->instances.size()}
            };
            
            // 添加包围盒信息
            auto bb = object->bounding_box();
            obj["bounding_box"] = {
                {"min_x", bb.min.x()},
                {"min_y", bb.min.y()},
                {"min_z", bb.min.z()},
                {"max_x", bb.max.x()},
                {"max_y", bb.max.y()},
                {"max_z", bb.max.z()}
            };
            
            model_json.push_back(obj);
        }
        
        return model_json.dump();
    } catch (const std::exception& e) {
        LOGE("Failed to get model info: %s", e.what());
        return "[]"; // 返回空数组
    }
}

bool OrcaSlicerApp::rotateModel(int object_id, float angle, int axis) {
    LOGI("Rotating model %d by %f degrees around axis %d", object_id, angle, axis);
    
    try {
        if (object_id < 0 || object_id >= static_cast<int>(model->objects.size())) {
            LOGE("Invalid object ID: %d", object_id);
            return false;
        }
        
        auto& object = model->objects[object_id];
        
        // 创建旋转矩阵
        Slic3r::Geometry::Transformation t;
        float angle_rad = Slic3r::Geometry::deg2rad(angle);
        
        switch (axis) {
            case 0: // X轴
                t.set_rotation({angle_rad, 0, 0});
                break;
            case 1: // Y轴
                t.set_rotation({0, angle_rad, 0});
                break;
            case 2: // Z轴
                t.set_rotation({0, 0, angle_rad});
                break;
            default:
                LOGE("Invalid rotation axis: %d", axis);
                return false;
        }
        
        // 应用变换到所有实例
        for (auto& instance : object->instances) {
            instance->transform = t * instance->transform;
        }
        
        LOGI("Model rotated successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to rotate model: %s", e.what());
        return false;
    }
}

bool OrcaSlicerApp::scaleModel(int object_id, float scale) {
    LOGI("Scaling model %d by factor %f", object_id, scale);
    
    try {
        if (object_id < 0 || object_id >= static_cast<int>(model->objects.size())) {
            LOGE("Invalid object ID: %d", object_id);
            return false;
        }
        
        if (scale <= 0) {
            LOGE("Invalid scale factor: %f", scale);
            return false;
        }
        
        auto& object = model->objects[object_id];
        
        // 创建缩放矩阵
        Slic3r::Geometry::Transformation t;
        t.set_scaling_factor(scale);
        
        // 应用变换到所有实例
        for (auto& instance : object->instances) {
            instance->transform = t * instance->transform;
        }
        
        LOGI("Model scaled successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to scale model: %s", e.what());
        return false;
    }
}

bool OrcaSlicerApp::translateModel(int object_id, float x, float y, float z) {
    LOGI("Translating model %d by (%f, %f, %f)", object_id, x, y, z);
    
    try {
        if (object_id < 0 || object_id >= static_cast<int>(model->objects.size())) {
            LOGE("Invalid object ID: %d", object_id);
            return false;
        }
        
        auto& object = model->objects[object_id];
        
        // 创建平移矩阵
        Slic3r::Geometry::Transformation t;
        t.set_offset({x, y, z});
        
        // 应用变换到所有实例
        for (auto& instance : object->instances) {
            instance->transform = t * instance->transform;
        }
        
        LOGI("Model translated successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to translate model: %s", e.what());
        return false;
    }
}

bool OrcaSlicerApp::deleteModel(int object_id) {
    LOGI("Deleting model %d", object_id);
    
    try {
        if (object_id < 0 || object_id >= static_cast<int>(model->objects.size())) {
            LOGE("Invalid object ID: %d", object_id);
            return false;
        }
        
        // 删除对象
        model->delete_object(model->objects[object_id]);
        
        LOGI("Model deleted successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to delete model: %s", e.what());
        return false;
    }
}

std::string OrcaSlicerApp::getSlicePreviewInfo() {
    LOGI("Getting slice preview information");
    
    try {
        if (!print->is_step_done(Slic3r::PrintObjectStep::Slice)) {
            LOGE("Slicing has not been performed yet");
            return "{}";
        }
        
        json preview_json;
        
        // 获取切片信息
        preview_json["layer_count"] = print->objects().front()->layers().size();
        preview_json["total_time"] = print->get_total_print_time();
        preview_json["filament_used"] = print->get_total_filament_used();
        
        // 获取层信息
        json layers = json::array();
        for (const auto& layer : print->objects().front()->layers()) {
            json layer_info = {
                {"height", layer->height},
                {"slice_z", layer->slice_z},
                {"print_z", layer->print_z}
            };
            layers.push_back(layer_info);
        }
        preview_json["layers"] = layers;
        
        return preview_json.dump();
    } catch (const std::exception& e) {
        LOGE("Failed to get slice preview info: %s", e.what());
        return "{}";
    }
}

std::string OrcaSlicerApp::getGCodePreviewInfo() {
    LOGI("Getting G-code preview information");
    
    try {
        if (!print->is_step_done(Slic3r::PrintObjectStep::GCodePath)) {
            LOGE("G-code paths have not been generated yet");
            return "{}";
        }
        
        json preview_json;
        
        // 获取G代码预览信息
        Slic3r::GCode::PreviewData preview_data;
        print->get_gcode_preview_data(preview_data);
        
        // 提取基本信息
        preview_json["extrusion_paths_count"] = preview_data.extrusion_paths.size();
        preview_json["travel_paths_count"] = preview_data.travel_paths.size();
        preview_json["retraction_count"] = preview_data.retraction_points.size();
        
        return preview_json.dump();
    } catch (const std::exception& e) {
        LOGE("Failed to get G-code preview info: %s", e.what());
        return "{}";
    }
}

bool OrcaSlicerApp::exportGCodeToFile(const std::string& file_path) {
    LOGI("Exporting G-code to file: %s", file_path.c_str());
    
    try {
        if (!print->is_step_done(Slic3r::PrintObjectStep::GCodePath)) {
            LOGE("G-code has not been generated yet");
            return false;
        }
        
        // 导出G代码到文件
        print->export_gcode(file_path, nullptr);
        
        LOGI("G-code exported successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to export G-code: %s", e.what());
        return false;
    }
}

// C接口实现，供JNI调用
extern "C" {

    void* orca_slicer_init(const char* data_path) {
        if (!data_path) {
            LOGE("Invalid data path (null)");
            return nullptr;
        }
        
        try {
            return new OrcaSlicerApp(data_path);
        } catch (const std::exception& e) {
            LOGE("Failed to initialize OrcaSlicerApp: %s", e.what());
            return nullptr;
        }
    }
    
    void orca_slicer_set_surface(void* instance, ANativeWindow* window, int width, int height) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        app->setSurface(window, width, height);
    }
    
    void orca_slicer_surface_destroyed(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        app->surfaceDestroyed();
    }
    
    void orca_slicer_resume(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        app->resume();
    }
    
    void orca_slicer_pause(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        app->pause();
    }
    
    void orca_slicer_destroy(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        delete app;
    }
    
    bool orca_slicer_import_model(void* instance, const char* model_path) {
        if (!instance || !model_path) {
            LOGE("Invalid parameters: instance=%p, model_path=%p", instance, model_path);
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->importModel(model_path);
    }
    
    bool orca_slicer_start_slicing(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->startSlicing();
    }
    
    // 新增C接口实现
    const char* orca_slicer_get_printers_list(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return "[]";
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        // 注意：这里返回的是临时字符串，在实际应用中需要考虑内存管理
        static std::string result;
        result = app->getPrintersList();
        return result.c_str();
    }
    
    const char* orca_slicer_get_materials_list(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return "[]";
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        static std::string result;
        result = app->getMaterialsList();
        return result.c_str();
    }
    
    const char* orca_slicer_get_print_settings_list(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return "[]";
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        static std::string result;
        result = app->getPrintSettingsList();
        return result.c_str();
    }
    
    bool orca_slicer_select_printer(void* instance, const char* printer_name) {
        if (!instance || !printer_name) {
            LOGE("Invalid parameters: instance=%p, printer_name=%p", instance, printer_name);
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->selectPrinter(printer_name);
    }
    
    bool orca_slicer_select_material(void* instance, const char* material_name) {
        if (!instance || !material_name) {
            LOGE("Invalid parameters: instance=%p, material_name=%p", instance, material_name);
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->selectMaterial(material_name);
    }
    
    bool orca_slicer_select_print_settings(void* instance, const char* print_settings_name) {
        if (!instance || !print_settings_name) {
            LOGE("Invalid parameters: instance=%p, print_settings_name=%p", instance, print_settings_name);
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->selectPrintSettings(print_settings_name);
    }
    
    const char* orca_slicer_get_model_info(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return "[]";
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        static std::string result;
        result = app->getModelInfo();
        return result.c_str();
    }
    
    bool orca_slicer_rotate_model(void* instance, int object_id, float angle, int axis) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->rotateModel(object_id, angle, axis);
    }
    
    bool orca_slicer_scale_model(void* instance, int object_id, float scale) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->scaleModel(object_id, scale);
    }
    
    bool orca_slicer_translate_model(void* instance, int object_id, float x, float y, float z) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->translateModel(object_id, x, y, z);
    }
    
    bool orca_slicer_delete_model(void* instance, int object_id) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->deleteModel(object_id);
    }
    
    const char* orca_slicer_get_slice_preview_info(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return "{}";
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        static std::string result;
        result = app->getSlicePreviewInfo();
        return result.c_str();
    }
    
    const char* orca_slicer_get_gcode_preview_info(void* instance) {
        if (!instance) {
            LOGE("Invalid instance (null)");
            return "{}";
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        static std::string result;
        result = app->getGCodePreviewInfo();
        return result.c_str();
    }
    
    bool orca_slicer_export_gcode_to_file(void* instance, const char* file_path) {
        if (!instance || !file_path) {
            LOGE("Invalid parameters: instance=%p, file_path=%p", instance, file_path);
            return false;
        }
        
        auto app = static_cast<OrcaSlicerApp*>(instance);
        return app->exportGCodeToFile(file_path);
    }
}