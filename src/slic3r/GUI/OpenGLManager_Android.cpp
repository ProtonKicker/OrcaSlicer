#include "OpenGLManager.hpp"
#include "GUI_App.hpp"

#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/predicate.hpp>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

namespace Slic3r {
namespace GUI {

// Android-specific OpenGL context and surface management
class AndroidGLContext
{
public:
    AndroidGLContext() : 
        m_display(EGL_NO_DISPLAY),
        m_context(EGL_NO_CONTEXT),
        m_surface(EGL_NO_SURFACE),
        m_config(nullptr)
    {}

    ~AndroidGLContext()
    {
        destroy();
    }

    bool init(ANativeWindow* window)
    {
        // 1. Initialize EGL display
        m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_display == EGL_NO_DISPLAY) {
            BOOST_LOG_TRIVIAL(error) << "Failed to get EGL display";
            return false;
        }

        // 2. Initialize EGL
        EGLint major, minor;
        if (eglInitialize(m_display, &major, &minor) != EGL_TRUE) {
            BOOST_LOG_TRIVIAL(error) << "Failed to initialize EGL";
            return false;
        }
        BOOST_LOG_TRIVIAL(info) << "EGL initialized: version " << major << "." << minor;

        // 3. Choose EGL config
        const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,
            EGL_SAMPLE_BUFFERS, 1,
            EGL_SAMPLES, 4,
            EGL_NONE
        };

        EGLint num_configs;
        if (eglChooseConfig(m_display, attribs, &m_config, 1, &num_configs) != EGL_TRUE || num_configs == 0) {
            BOOST_LOG_TRIVIAL(error) << "Failed to choose EGL config";
            return false;
        }

        // 4. Create EGL surface
        m_surface = eglCreateWindowSurface(m_display, m_config, window, nullptr);
        if (m_surface == EGL_NO_SURFACE) {
            BOOST_LOG_TRIVIAL(error) << "Failed to create EGL surface";
            return false;
        }

        // 5. Create EGL context
        const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
        };

        m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, context_attribs);
        if (m_context == EGL_NO_CONTEXT) {
            BOOST_LOG_TRIVIAL(error) << "Failed to create EGL context";
            return false;
        }

        return true;
    }

    bool make_current()
    {
        if (m_display == EGL_NO_DISPLAY || m_context == EGL_NO_CONTEXT || m_surface == EGL_NO_SURFACE) {
            BOOST_LOG_TRIVIAL(error) << "Cannot make current with invalid EGL objects";
            return false;
        }

        if (eglMakeCurrent(m_display, m_surface, m_surface, m_context) != EGL_TRUE) {
            BOOST_LOG_TRIVIAL(error) << "Failed to make EGL context current";
            return false;
        }

        return true;
    }

    void swap_buffers()
    {
        if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE) {
            eglSwapBuffers(m_display, m_surface);
        }
    }

    void destroy()
    {
        if (m_display != EGL_NO_DISPLAY) {
            if (m_context != EGL_NO_CONTEXT) {
                eglDestroyContext(m_display, m_context);
                m_context = EGL_NO_CONTEXT;
            }

            if (m_surface != EGL_NO_SURFACE) {
                eglDestroySurface(m_display, m_surface);
                m_surface = EGL_NO_SURFACE;
            }

            eglTerminate(m_display);
            m_display = EGL_NO_DISPLAY;
        }
    }

private:
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;
    EGLConfig  m_config;
};

// Android-specific implementation of OpenGL initialization
bool OpenGLManager::init_gl(bool popup_error)
{
    if (!m_gl_initialized) {
        // For Android, we use GLES3 which doesn't require GLEW
        m_gl_initialized = true;

        // Check OpenGL ES version
        const char* version = (const char*)glGetString(GL_VERSION);
        BOOST_LOG_TRIVIAL(info) << "OpenGL ES version: " << (version ? version : "unknown");

        // Check if we have the minimum required version (OpenGL ES 3.0)
        bool valid_version = false;
        if (version) {
            // OpenGL ES version string format: "OpenGL ES <major>.<minor> <vendor-specific info>"
            if (strstr(version, "OpenGL ES") != nullptr) {
                const char* ver_ptr = version + 10; // Skip "OpenGL ES "
                int major = ver_ptr[0] - '0';
                valid_version = (major >= 3);
            }
        }

        if (!valid_version) {
            BOOST_LOG_TRIVIAL(error) << "OpenGL ES version is lower than 3.0";
            if (popup_error) {
                // In Android, we would show a dialog or toast message
                // This would be implemented in the Java/Kotlin layer
            }
            return false;
        }

        // Initialize shaders
        auto [result, error] = m_shaders_manager.init();
        if (!result) {
            BOOST_LOG_TRIVIAL(error) << "Unable to load shaders: " << error;
            if (popup_error) {
                // Show error in Android UI
            }
            return false;
        }

        // Check for texture compression support
        s_compressed_textures_supported = false;
        if (glGetString(GL_EXTENSIONS)) {
            const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
            if (strstr(extensions, "GL_EXT_texture_compression_s3tc") != nullptr) {
                s_compressed_textures_supported = true;
            }
        }

        // For Android, we always use framebuffer objects as they are core in GLES3
        s_framebuffers_type = EFramebufferType::Arb;
    }

    return true;
}

// Android-specific implementation of OpenGL context initialization
wxGLContext* OpenGLManager::init_glcontext(wxGLCanvas& canvas)
{
    // On Android, we don't use wxGLContext
    // Instead, we use EGL context which is managed separately
    // This is just a stub to maintain API compatibility
    return nullptr;
}

// Android-specific implementation of wxGLCanvas creation
wxGLCanvas* OpenGLManager::create_wxglcanvas(wxWindow& parent)
{
    // On Android, we don't use wxGLCanvas
    // Instead, we use ANativeWindow which is provided by the Android system
    // This is just a stub to maintain API compatibility
    return nullptr;
}

// Android-specific implementation of multisample detection
void OpenGLManager::detect_multisample(int* attribList)
{
    // On Android, we always try to enable multisampling if the device supports it
    // The actual support is determined when creating the EGL config
    s_multisample = EMultisampleState::Enabled;
}

} // namespace GUI
} // namespace Slic3r

#endif // __ANDROID__