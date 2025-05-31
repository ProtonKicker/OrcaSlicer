#include <jni.h>
#include <android/native_window.h>
#include <android/log.h>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <vector>
#include <chrono>

#define LOG_TAG "OrcaSlicerAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 前向声明
namespace Slic3r {
    namespace GUI {
        class AndroidGLContext;
    }
}

// 前向声明C函数，这些函数在OrcaSlicerJNI.cpp中实现
extern "C" {
    // 这个函数在OrcaSlicerJNI.cpp中实现，用于通知Java层切片完成
    void orca_slicer_notify_slicing_finished(bool success);
}

// OrcaSlicer应用程序实例类
class OrcaSlicerApp {
    public:
        OrcaSlicerApp(const std::string& data_path);
        ~OrcaSlicerApp();

        bool initialize();
        void setSurface(ANativeWindow* window, int width, int height);
        void surfaceDestroyed();
        void resume();
        void pause();
        void renderLoop();
        void stopRenderLoop();
        
        // 模型导入和切片功能
        void importModel(const std::string& model_path);
        bool startSlicing();

    private:
        std::string m_data_path;
        ANativeWindow* m_window;
        int m_width;
        int m_height;
        
        std::unique_ptr<Slic3r::GUI::AndroidGLContext> m_gl_context;
        
        std::thread m_render_thread;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_running;
        bool m_surface_ready;
        bool m_app_active;
        
        // 模型和切片状态
        bool m_model_loaded;
        bool m_slicing_in_progress;
};

// 实现OrcaSlicerApp类
OrcaSlicerApp::OrcaSlicerApp(const std::string& data_path)
    : m_data_path(data_path)
    , m_window(nullptr)
    , m_width(0)
    , m_height(0)
    , m_running(false)
    , m_surface_ready(false)
    , m_app_active(false)
    , m_model_loaded(false)
    , m_slicing_in_progress(false)
{
    LOGI("OrcaSlicerApp created with data path: %s", m_data_path.c_str());
}

OrcaSlicerApp::~OrcaSlicerApp() {
    // 停止渲染线程
    stopRenderLoop();
    
    // 销毁OpenGL上下文
    if (m_gl_context) {
        m_gl_context->destroy();
        m_gl_context.reset();
    }
    
    // 释放ANativeWindow
    if (m_window) {
        ANativeWindow_release(m_window);
        m_window = nullptr;
    }
    
    LOGI("OrcaSlicerApp destroyed");
}

bool OrcaSlicerApp::initialize() {
    LOGI("Initializing OrcaSlicerApp");
    
    // 在这里初始化OrcaSlicer的核心组件
    // 这将在未来实现
    
    return true;
}

void OrcaSlicerApp::setSurface(ANativeWindow* window, int width, int height) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_window) {
        ANativeWindow_release(m_window);
    }
    
    m_window = window;
    m_width = width;
    m_height = height;
    m_surface_ready = (window != nullptr);
    
    LOGI("Surface set: %dx%d, ready: %d", width, height, m_surface_ready);
    
    if (m_surface_ready) {
        // 初始化OpenGL上下文
        if (!m_gl_context) {
            m_gl_context = std::make_unique<Slic3r::GUI::AndroidGLContext>();
        }
        
        if (m_gl_context->init(window)) {
            LOGI("OpenGL context initialized successfully");
            
            // 设置当前上下文
            if (m_gl_context->make_current()) {
                LOGI("OpenGL context made current");
                
                // 设置视口
                glViewport(0, 0, width, height);
            } else {
                LOGE("Failed to make OpenGL context current");
            }
        } else {
            LOGE("Failed to initialize OpenGL context");
        }
        
        // 启动渲染线程
        if (!m_running) {
            m_running = true;
            m_render_thread = std::thread(&OrcaSlicerApp::renderLoop, this);
        } else {
            m_cv.notify_one();
        }
    }
}

void OrcaSlicerApp::surfaceDestroyed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 标记表面不可用，这会导致渲染线程暂停渲染
    m_surface_ready = false;
    
    // 释放OpenGL上下文资源
    if (m_gl_context) {
        m_gl_context->destroy();
    }
    
    // 释放ANativeWindow
    if (m_window) {
        ANativeWindow_release(m_window);
        m_window = nullptr;
    }
    
    LOGI("Surface destroyed");
}

void OrcaSlicerApp::resume() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_app_active = true;
    m_cv.notify_one();
    LOGI("App resumed");
}

void OrcaSlicerApp::pause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_app_active = false;
    LOGI("App paused");
}

void OrcaSlicerApp::renderLoop() {
    LOGI("Render thread started");
    
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // 等待表面准备好且应用程序处于活动状态
        m_cv.wait(lock, [this]() {
            return !m_running || (m_surface_ready && m_app_active);
        });
        
        if (!m_running) break;
        
        // 确保OpenGL上下文是当前的
        if (m_gl_context && m_surface_ready) {
            // 设置当前上下文
            if (m_gl_context->make_current()) {
                // 清除屏幕
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // 设置清除颜色为深青色
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                // 在这里执行实际的渲染
                // 这将在未来实现更复杂的渲染逻辑
                
                // 交换缓冲区
                m_gl_context->swap_buffers();
            }
        }
        
        // 解锁互斥锁，允许其他线程访问共享数据
        lock.unlock();
        
        // 限制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    LOGI("Render thread stopped");
}

void OrcaSlicerApp::stopRenderLoop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
        m_cv.notify_one();
    }
    
    if (m_render_thread.joinable()) {
        m_render_thread.join();
    }
}

// 添加模型导入和切片功能
bool OrcaSlicerApp::importModel(const std::string& model_path) {
    LOGI("Importing model from: %s", model_path.c_str());
    
    // 检查文件路径是否为空
    if (model_path.empty()) {
        LOGE("Model path is empty");
        return false;
    }
    
    // 检查文件是否存在
    // 在实际实现中，应该使用平台特定的文件存在检查
    // 这里简化处理，假设文件存在
    
    // 检查文件扩展名
    std::string extension;
    size_t dot_pos = model_path.find_last_of(".");
    if (dot_pos != std::string::npos) {
        extension = model_path.substr(dot_pos + 1);
        // 转换为小写
        std::transform(extension.begin(), extension.end(), extension.begin(), 
                       [](unsigned char c) { return std::tolower(c); });
    }
    
    // 检查是否是支持的文件类型
    bool supported = false;
    const std::vector<std::string> supported_extensions = {"stl", "obj", "3mf", "amf"};
    for (const auto& ext : supported_extensions) {
        if (extension == ext) {
            supported = true;
            break;
        }
    }
    
    if (!supported) {
        LOGE("Unsupported file format: %s", extension.c_str());
        return false;
    }
    
    // TODO: 实现实际的模型导入逻辑
    // 在实际实现中，这里应该调用OrcaSlicer的模型导入功能
    
    // 模拟导入成功
    m_model_loaded = true;
    LOGI("Model imported successfully");
    
    return true; // 返回导入是否成功
}

bool OrcaSlicerApp::startSlicing() {
    LOGI("Starting slicing process");
    
    // 检查是否已经有模型被导入
    if (!m_model_loaded) {
        LOGE("No model loaded, cannot start slicing");
        return false;
    }
    
    // 检查是否已经在切片中
    if (m_slicing_in_progress) {
        LOGE("Slicing already in progress");
        return false;
    }
    
    m_slicing_in_progress = true;
    
    // 创建一个新线程来执行切片过程
    std::thread slicing_thread([this]() {
        LOGI("Slicing thread started");
        
        // 模拟切片过程
        // 在实际实现中，这里应该调用OrcaSlicer的核心切片功能
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        bool success = true; // 模拟切片成功
        
        // 切片完成，重置标志
        m_slicing_in_progress = false;
        
        // 通知Java层切片已完成
        LOGI("Slicing completed, notifying Java layer");
        orca_slicer_notify_slicing_finished(success);
    });
    
    // 分离线程，让它在后台运行
    slicing_thread.detach();
    
    return true; // 返回切片是否成功启动
}

// 导出的C函数，供JNI调用
extern "C" {

    void* orca_slicer_init(const char* data_path) {
        auto* app = new OrcaSlicerApp(data_path);
        if (!app->initialize()) {
            delete app;
            return nullptr;
        }
        return app;
    }

    void orca_slicer_set_surface(void* instance, ANativeWindow* window, int width, int height) {
        auto* app = static_cast<OrcaSlicerApp*>(instance);
        if (app) {
            app->setSurface(window, width, height);
        }
    }

    void orca_slicer_surface_destroyed(void* instance) {
        auto* app = static_cast<OrcaSlicerApp*>(instance);
        if (app) {
            app->surfaceDestroyed();
        }
    }

    void orca_slicer_resume(void* instance) {
        auto* app = static_cast<OrcaSlicerApp*>(instance);
        if (app) {
            app->resume();
        }
    }

    void orca_slicer_pause(void* instance) {
        auto* app = static_cast<OrcaSlicerApp*>(instance);
        if (app) {
            app->pause();
        }
    }

    void orca_slicer_destroy(void* instance) {
        auto* app = static_cast<OrcaSlicerApp*>(instance);
        if (app) {
            delete app;
        }
    }
    
    bool orca_slicer_import_model(void* instance, const char* model_path) {
        if (!instance) {
            LOGE("Invalid instance pointer in orca_slicer_import_model");
            return false;
        }
        
        if (!model_path) {
            LOGE("Invalid model path in orca_slicer_import_model");
            return false;
        }
        
        OrcaSlicerApp* app = static_cast<OrcaSlicerApp*>(instance);
        return app->importModel(model_path);
    }
    
    bool orca_slicer_start_slicing(void* instance) {
        auto* app = static_cast<OrcaSlicerApp*>(instance);
        if (!app) {
            LOGE("Invalid app instance");
            return false;
        }
        
        return app->startSlicing();
    }
}