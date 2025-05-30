#include "libslic3r/libslic3r.h"
#include "OpenGLManager.hpp"

#include "GUI.hpp"
#include "I18N.hpp"
#include "3DScene.hpp"

#include "libslic3r/Platform.hpp"

#ifndef __ANDROID__
#include <GL/glew.h>
#else
#include <GLES3/gl3.h>
#endif

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/log/trivial.hpp>

#include <wx/glcanvas.h>
#include <wx/msgdlg.h>

#ifdef __APPLE__
// Part of hack to remove crash when closing the application on OSX 10.9.5 when building against newer wxWidgets
#include <wx/platinfo.h>

#include "../Utils/MacDarkMode.hpp"
#endif // __APPLE__

namespace Slic3r {
namespace GUI {

// A safe wrapper around glGetString to report a "N/A" string in case glGetString returns nullptr.
std::string gl_get_string_safe(GLenum param, const std::string& default_value)
{
    const char* value = (const char*)::glGetString(param);
    return std::string((value != nullptr) ? value : default_value);
}

const std::string& OpenGLManager::GLInfo::get_version() const
{
    if (!m_detected)
        detect();

    return m_version;
}

const std::string& OpenGLManager::GLInfo::get_glsl_version() const
{
    if (!m_detected)
        detect();

    return m_glsl_version;
}

const std::string& OpenGLManager::GLInfo::get_vendor() const
{
    if (!m_detected)
        detect();

    return m_vendor;
}

const std::string& OpenGLManager::GLInfo::get_renderer() const
{
    if (!m_detected)
        detect();

    return m_renderer;
}

bool OpenGLManager::GLInfo::is_mesa() const
{
    return boost::icontains(m_version, "mesa");
}

int OpenGLManager::GLInfo::get_max_tex_size() const
{
    if (!m_detected)
        detect();

    // clamp to avoid the texture generation become too slow and use too much GPU memory
#ifdef __APPLE__
    // and use smaller texture for non retina systems
    return (Slic3r::GUI::mac_max_scaling_factor() > 1.0) ? std::min(m_max_tex_size, 8192) : std::min(m_max_tex_size / 2, 4096);
#else
    // and use smaller texture for older OpenGL versions
    return is_version_greater_or_equal_to(3, 0) ? std::min(m_max_tex_size, 8192) : std::min(m_max_tex_size / 2, 4096);
#endif // __APPLE__
}

float OpenGLManager::GLInfo::get_max_anisotropy() const
{
    if (!m_detected)
        detect();

    return m_max_anisotropy;
}

void OpenGLManager::GLInfo::detect() const
{
    *const_cast<std::string*>(&m_version) = gl_get_string_safe(GL_VERSION, "N/A");
    *const_cast<std::string*>(&m_glsl_version) = gl_get_string_safe(GL_SHADING_LANGUAGE_VERSION, "N/A");
    *const_cast<std::string*>(&m_vendor) = gl_get_string_safe(GL_VENDOR, "N/A");
    *const_cast<std::string*>(&m_renderer) = gl_get_string_safe(GL_RENDERER, "N/A");

    BOOST_LOG_TRIVIAL(info) << boost::format("got opengl version %1%, glsl version %2%, vendor %3%")%m_version %m_glsl_version %m_vendor<< std::endl;

    int* max_tex_size = const_cast<int*>(&m_max_tex_size);
    glsafe(::glGetIntegerv(GL_MAX_TEXTURE_SIZE, max_tex_size));

    *max_tex_size /= 2;

    if (Slic3r::total_physical_memory() / (1024 * 1024 * 1024) < 6)
        *max_tex_size /= 2;

    if (GLEW_EXT_texture_filter_anisotropic) {
        float* max_anisotropy = const_cast<float*>(&m_max_anisotropy);
        glsafe(::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy));
    }
    *const_cast<bool*>(&m_detected) = true;
}

static bool version_greater_or_equal_to(const std::string& version, unsigned int major, unsigned int minor)
{
    if (version == "N/A")
        return false;

    std::vector<std::string> tokens;
    boost::split(tokens, version, boost::is_any_of(" "), boost::token_compress_on);

    if (tokens.empty())
        return false;

    std::vector<std::string> numbers;
    boost::split(numbers, tokens[0], boost::is_any_of("."), boost::token_compress_on);

    unsigned int gl_major = 0;
    unsigned int gl_minor = 0;

    if (numbers.size() > 0)
        gl_major = ::atoi(numbers[0].c_str());

    if (numbers.size() > 1)
        gl_minor = ::atoi(numbers[1].c_str());

    if (gl_major < major)
        return false;
    else if (gl_major > major)
        return true;
    else
        return gl_minor >= minor;
}

bool OpenGLManager::GLInfo::is_version_greater_or_equal_to(unsigned int major, unsigned int minor) const
{
    if (!m_detected)
        detect();

    return version_greater_or_equal_to(m_version, major, minor);
}

bool OpenGLManager::GLInfo::is_glsl_version_greater_or_equal_to(unsigned int major, unsigned int minor) const
{
    if (!m_detected)
        detect();

    return version_greater_or_equal_to(m_glsl_version, major, minor);
}

// If formatted for github, plaintext with OpenGL extensions enclosed into <details>.
// Otherwise HTML formatted for the system info dialog.
std::string OpenGLManager::GLInfo::to_string(bool for_github) const
{
    if (!m_detected)
        detect();

    std::stringstream out;

    const bool format_as_html = ! for_github;
    std::string h2_start = format_as_html ? "<b>" : "";
    std::string h2_end = format_as_html ? "</b>" : "";
    std::string b_start = format_as_html ? "<b>" : "";
    std::string b_end = format_as_html ? "</b>" : "";
    std::string line_end = format_as_html ? "<br>" : "\n";

    out << h2_start << "OpenGL installation" << h2_end << line_end;
    out << b_start << "GL version:   " << b_end << m_version << line_end;
    out << b_start << "Vendor:       " << b_end << m_vendor << line_end;
    out << b_start << "Renderer:     " << b_end << m_renderer << line_end;
    out << b_start << "GLSL version: " << b_end << m_glsl_version << line_end;

    {
        std::vector<std::string> extensions_list;
        std::string extensions_str = gl_get_string_safe(GL_EXTENSIONS, "");
        boost::split(extensions_list, extensions_str, boost::is_any_of(" "), boost::token_compress_on);

        if (!extensions_list.empty()) {
            if (for_github)
                out << "<details>\n<summary>Installed extensions:</summary>\n";
            else
                out << h2_start << "Installed extensions:" << h2_end << line_end;

            std::sort(extensions_list.begin(), extensions_list.end());
            for (const std::string& ext : extensions_list)
                if (! ext.empty())
                    out << ext << line_end;

            if (for_github)
                out << "</details>\n";
        }
    }

    return out.str();
}

OpenGLManager::GLInfo OpenGLManager::s_gl_info;
bool OpenGLManager::s_compressed_textures_supported = false;
bool OpenGLManager::s_force_power_of_two_textures = false;
OpenGLManager::EMultisampleState OpenGLManager::s_multisample = OpenGLManager::EMultisampleState::Unknown;
OpenGLManager::EFramebufferType OpenGLManager::s_framebuffers_type = OpenGLManager::EFramebufferType::Unknown;

#ifdef __APPLE__
// Part of hack to remove crash when closing the application on OSX 10.9.5 when building against newer wxWidgets
OpenGLManager::OSInfo OpenGLManager::s_os_info;
#endif // __APPLE__

OpenGLManager::~OpenGLManager()
{
    m_shaders_manager.shutdown();

#ifdef __APPLE__
    // This is an ugly hack needed to solve the crash happening when closing the application on OSX 10.9.5 with newer wxWidgets
    // The crash is triggered inside wxGLContext destructor
    if (s_os_info.major != 10 || s_os_info.minor != 9 || s_os_info.micro != 5)
    {
#endif //__APPLE__
        if (m_context != nullptr)
            delete m_context;
#ifdef __APPLE__
    }
#endif //__APPLE__
}

bool OpenGLManager::init_gl(bool popup_error)
{
    if (!m_gl_initialized) {
#ifndef __ANDROID__
        GLenum result = glewInit();
        if (result != GLEW_OK) {
            BOOST_LOG_TRIVIAL(error) << "Unable to init glew library";
            return false;
        }
        //BOOST_LOG_TRIVIAL(info) << "glewInit Success."<< std::endl;
        m_gl_initialized = true;
        if (GLEW_EXT_texture_compression_s3tc)
            s_compressed_textures_supported = true;
        else
            s_compressed_textures_supported = false;

        if (GLEW_ARB_framebuffer_object) {
            s_framebuffers_type = EFramebufferType::Arb;
            BOOST_LOG_TRIVIAL(info) << "Found Framebuffer Type ARB."<< std::endl;
        }
        else if (GLEW_EXT_framebuffer_object) {
            BOOST_LOG_TRIVIAL(info) << "Found Framebuffer Type Ext."<< std::endl;
            s_framebuffers_type = EFramebufferType::Ext;
        }
        else {
            s_framebuffers_type = EFramebufferType::Unknown;
            BOOST_LOG_TRIVIAL(warning) << "Found Framebuffer Type unknown!"<< std::endl;
        }
#else
        // For Android, we use GLES3 which doesn't require GLEW
        m_gl_initialized = true;
        
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
#endif

#ifndef __ANDROID__
        bool valid_version = s_gl_info.is_version_greater_or_equal_to(2, 0);
        if (!valid_version) {
            BOOST_LOG_TRIVIAL(error) << "Found opengl version <= 2.0"<< std::endl;
            // Complain about the OpenGL version.
            if (popup_error) {
                wxString message = from_u8((boost::format(
                    _utf8(L("The application cannot run normally because OpenGL version is lower than 2.0.\n")))).str());
                message += "\n";
                message += _L("Please upgrade your graphics card driver.");
                wxMessageBox(message, _L("Unsupported OpenGL version"), wxOK | wxICON_ERROR);
            }
        }
#else
        // Check OpenGL ES version
        bool valid_version = false;
        const char* version = (const char*)glGetString(GL_VERSION);
        BOOST_LOG_TRIVIAL(info) << "OpenGL ES version: " << (version ? version : "unknown");
        
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
#endif

        if (valid_version)
        {
            // load shaders
            auto [result, error] = m_shaders_manager.init();
            if (!result) {
                BOOST_LOG_TRIVIAL(error) << "Unable to load shaders: "<<error<< std::endl;
                if (popup_error) {
                    wxString message = from_u8((boost::format(
                        _utf8(L("Unable to load shaders:\n%s"))) % error).str());
                    wxMessageBox(message, _L("Error loading shaders"), wxOK | wxICON_ERROR);
                }
            }
        }

#ifdef _WIN32
        // Since AMD driver version 22.7.1, there is probably some bug in the driver that causes the issue with the missing
        // texture of the bed (see: https://github.com/prusa3d/PrusaSlicer/issues/8417).
        // It seems that this issue only triggers when mipmaps are generated manually
        // (combined with a texture compression) with texture size not being power of two.
        // When mipmaps are generated through OpenGL function glGenerateMipmap() the driver works fine,
        // but the mipmap generation is quite slow on some machines.
        // There is no an easy way to detect the driver version without using Win32 API because the strings returned by OpenGL
        // have no standardized format, only some of them contain the driver version.
        // Until we do not know that driver will be fixed (if ever) we force the use of power of two textures on all cards
        // 1) containing the string 'Radeon' in the string returned by glGetString(GL_RENDERER)
        // 2) containing the string 'Custom' in the string returned by glGetString(GL_RENDERER)
        const auto& gl_info = OpenGLManager::get_gl_info();
        if (boost::contains(gl_info.get_vendor(), "ATI Technologies Inc.") &&
           (boost::contains(gl_info.get_renderer(), "Radeon") ||
            boost::contains(gl_info.get_renderer(), "Custom")))
            s_force_power_of_two_textures = true;
#endif // _WIN32
    }

    return true;
}

wxGLContext* OpenGLManager::init_glcontext(wxGLCanvas& canvas)
{
#ifndef __ANDROID__
    if (m_context == nullptr) {
        m_context = new wxGLContext(&canvas);

#ifdef __APPLE__
        // Part of hack to remove crash when closing the application on OSX 10.9.5 when building against newer wxWidgets
        s_os_info.major = wxPlatformInfo::Get().GetOSMajorVersion();
        s_os_info.minor = wxPlatformInfo::Get().GetOSMinorVersion();
        s_os_info.micro = wxPlatformInfo::Get().GetOSMicroVersion();
#endif //__APPLE__
    }
    return m_context;
#else
    // On Android, we don't use wxGLContext
    // Instead, we use EGL context which is managed separately
    // This is just a stub to maintain API compatibility
    return nullptr;
#endif
}

wxGLCanvas* OpenGLManager::create_wxglcanvas(wxWindow& parent)
{
#ifndef __ANDROID__
    int attribList[] = {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        // RGB channels each should be allocated with 8 bit depth. One should almost certainly get these bit depths by default.
        WX_GL_MIN_RED, 			8,
        WX_GL_MIN_GREEN, 		8,
        WX_GL_MIN_BLUE, 		8,
        // Requesting an 8 bit alpha channel. Interestingly, the NVIDIA drivers would most likely work with some alpha plane, but glReadPixels would not return
        // the alpha channel on NVIDIA if not requested when the GL context is created.
        WX_GL_MIN_ALPHA, 		8,
        WX_GL_DEPTH_SIZE, 		24,
        //BBS: turn on stencil buffer for outline
        WX_GL_STENCIL_SIZE,     8,
        WX_GL_SAMPLE_BUFFERS, 	GL_TRUE,
        WX_GL_SAMPLES, 			4,
        0
    };

    if (s_multisample == EMultisampleState::Unknown) {
        detect_multisample(attribList);
//        // debug output
//        std::cout << "Multisample " << (can_multisample() ? "enabled" : "disabled") << std::endl;
    }

    if (! can_multisample())
        attribList[12] = 0;

    return new wxGLCanvas(&parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
#else
    // On Android, we don't use wxGLCanvas
    // Instead, we use ANativeWindow which is provided by the Android system
    // This is just a stub to maintain API compatibility
    return nullptr;
#endif
}

void OpenGLManager::detect_multisample(int* attribList)
{
#ifndef __ANDROID__
    int wxVersion = wxMAJOR_VERSION * 10000 + wxMINOR_VERSION * 100 + wxRELEASE_NUMBER;
    bool enable_multisample = wxVersion >= 30003;
    s_multisample =
        enable_multisample &&
        // Disable multi-sampling on ChromeOS, as the OpenGL virtualization swaps Red/Blue channels with multi-sampling enabled,
        // at least on some platforms.
        platform_flavor() != PlatformFlavor::LinuxOnChromium &&
        wxGLCanvas::IsDisplaySupported(attribList)
        ? EMultisampleState::Enabled : EMultisampleState::Disabled;
    // Alternative method: it was working on previous version of wxWidgets but not with the latest, at least on Windows
    // s_multisample = enable_multisample && wxGLCanvas::IsExtensionSupported("WGL_ARB_multisample");
#else
    // On Android, we always try to enable multisampling if the device supports it
    // The actual support is determined when creating the EGL config
    s_multisample = EMultisampleState::Enabled;
#endif
}
} // namespace GUI
} // namespace Slic3r
