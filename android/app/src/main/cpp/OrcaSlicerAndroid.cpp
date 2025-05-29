#include <jni.h>
#include <android/native_window.h>
#include <android/log.h>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#define LOG_TAG "OrcaSlicerAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 前向声明
namespace Slic3r {
    namespace GUI {
        class AndroidGLContext;
    }
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
{
    LOGI("OrcaSlicerApp created with data path: %s", m_data_path.c_str());
}

OrcaSlicerApp::~OrcaSlicerApp() {
    stopRenderLoop();
    
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
    
    if (m_surface_ready && !m_running) {
        m_running = true;
        m_render_thread = std::thread(&OrcaSlicerApp::renderLoop, this);
    } else if (m_surface_ready) {
        m_cv.notify_one();
    }
}

void OrcaSlicerApp::surfaceDestroyed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_window) {
        ANativeWindow_release(m_window);
        m_window = nullptr;
    }
    
    m_surface_ready = false;
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
        
        // 在这里执行渲染
        // 这将在未来实现
        
        // 模拟渲染帧率
        lock.unlock();
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
}