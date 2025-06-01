#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <string>
#include <map>

#define LOG_TAG "OrcaSlicerJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 全局变量
static JavaVM* g_jvm = nullptr;
static jobject g_main_activity = nullptr;

// 辅助函数，用于从其他线程调用Java方法
void CallJavaMethod(const char* method_name, bool success) {
    JNIEnv* env;
    bool attached = false;
    
    // 获取JNIEnv
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (status < 0) {
        status = g_jvm->AttachCurrentThread(&env, nullptr);
        if (status < 0) {
            LOGE("Failed to attach thread to JVM");
            return;
        }
        attached = true;
    }
    
    // 查找MainActivity类
    jclass main_activity_class = env->GetObjectClass(g_main_activity);
    if (!main_activity_class) {
        LOGE("Failed to find MainActivity class");
        if (attached) g_jvm->DetachCurrentThread();
        return;
    }
    
    // 查找方法ID
    jmethodID method_id = env->GetMethodID(main_activity_class, method_name, "(Z)V");
    if (!method_id) {
        LOGE("Failed to find method %s", method_name);
        if (attached) g_jvm->DetachCurrentThread();
        return;
    }
    
    // 调用方法
    env->CallVoidMethod(g_main_activity, method_id, success);
    
    // 如果是附加的线程，分离它
    if (attached) g_jvm->DetachCurrentThread();
}

// 前向声明C函数，这些函数在OrcaSlicerAndroid.cpp中实现
extern "C" {
    void* orca_slicer_init(const char* data_path);
    void orca_slicer_set_surface(void* instance, ANativeWindow* window, int width, int height);
    void orca_slicer_surface_destroyed(void* instance);
    void orca_slicer_resume(void* instance);
    void orca_slicer_pause(void* instance);
    void orca_slicer_destroy(void* instance);
    bool orca_slicer_import_model(void* instance, const char* model_path);
    bool orca_slicer_start_slicing(void* instance);
    
    // 新增函数声明
    const char* orca_slicer_get_printers_list(void* instance);
    const char* orca_slicer_get_materials_list(void* instance);
    const char* orca_slicer_get_print_settings_list(void* instance);
    bool orca_slicer_select_printer(void* instance, const char* printer_name);
    bool orca_slicer_select_material(void* instance, const char* material_name);
    bool orca_slicer_select_print_settings(void* instance, const char* print_settings_name);
    const char* orca_slicer_get_model_info(void* instance);
    bool orca_slicer_rotate_model(void* instance, int object_id, float angle, int axis);
    bool orca_slicer_scale_model(void* instance, int object_id, float scale);
    bool orca_slicer_translate_model(void* instance, int object_id, float x, float y, float z);
    bool orca_slicer_delete_model(void* instance, int object_id);
    const char* orca_slicer_get_slice_preview_info(void* instance);
    const char* orca_slicer_get_gcode_preview_info(void* instance);
    bool orca_slicer_export_gcode_to_file(void* instance, const char* file_path);
}

// 这个函数在OrcaSlicerAndroid.cpp中实现，用于通知Java层切片完成
extern "C" void orca_slicer_notify_slicing_finished(bool success) {
    CallJavaMethod("onSlicingFinished", success);
}

// JNI加载时调用
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

// JNI方法实现
extern "C" {

    JNIEXPORT jlong JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeInit(JNIEnv* env, jobject thiz, jstring data_path, jobject activity) {
        // 保存MainActivity的全局引用
        if (g_main_activity) {
            env->DeleteGlobalRef(g_main_activity);
        }
        g_main_activity = env->NewGlobalRef(activity);
        
        // 转换Java字符串到C字符串
        const char* path = env->GetStringUTFChars(data_path, nullptr);
        
        // 初始化OrcaSlicer
        g_orca_slicer_instance = orca_slicer_init(path);
        
        // 释放字符串
        env->ReleaseStringUTFChars(data_path, path);
        
        // 返回实例指针作为jlong
        return reinterpret_cast<jlong>(g_orca_slicer_instance);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeSetThreadCount(JNIEnv* env, jobject thiz, jint thread_count) {
        // 设置线程数的实现将在未来添加
        LOGI("Setting thread count to %d", thread_count);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeSetExpertMode(JNIEnv* env, jobject thiz, jboolean expert_mode) {
        // 设置专家模式的实现将在未来添加
        LOGI("Setting expert mode to %d", expert_mode);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeSetLanguage(JNIEnv* env, jobject thiz, jstring language) {
        // 设置语言的实现将在未来添加
        const char* lang = env->GetStringUTFChars(language, nullptr);
        LOGI("Setting language to %s", lang);
        env->ReleaseStringUTFChars(language, lang);
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeImportModel(JNIEnv* env, jobject thiz, jstring model_path) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        const char* path = env->GetStringUTFChars(model_path, nullptr);
        bool result = orca_slicer_import_model(g_orca_slicer_instance, path);
        env->ReleaseStringUTFChars(model_path, path);
        
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeStartSlicing(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        return orca_slicer_start_slicing(g_orca_slicer_instance) ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeApplySettings(JNIEnv* env, jobject thiz) {
        // 应用设置的实现将在未来添加
        LOGI("Applying settings");
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jobject surface) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return;
        }
        
        // 获取原生窗口
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            LOGE("Failed to get native window from surface");
            return;
        }
        
        // 获取窗口尺寸
        int width = ANativeWindow_getWidth(window);
        int height = ANativeWindow_getHeight(window);
        
        // 设置表面
        orca_slicer_set_surface(g_orca_slicer_instance, window, width, height);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jobject surface, jint width, jint height) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return;
        }
        
        // 获取原生窗口
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            LOGE("Failed to get native window from surface");
            return;
        }
        
        // 设置表面
        orca_slicer_set_surface(g_orca_slicer_instance, window, width, height);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeOnSurfaceDestroyed(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return;
        }
        
        orca_slicer_surface_destroyed(g_orca_slicer_instance);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeOnResume(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return;
        }
        
        orca_slicer_resume(g_orca_slicer_instance);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeOnPause(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return;
        }
        
        orca_slicer_pause(g_orca_slicer_instance);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeDestroy(JNIEnv* env, jobject thiz) {
        if (g_orca_slicer_instance) {
            orca_slicer_destroy(g_orca_slicer_instance);
            g_orca_slicer_instance = nullptr;
        }
        
        // 释放MainActivity的全局引用
        if (g_main_activity) {
            env->DeleteGlobalRef(g_main_activity);
            g_main_activity = nullptr;
        }
    }

    // 新增JNI方法实现
    JNIEXPORT jstring JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeGetPrintersList(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return env->NewStringUTF("[]");
        }
        
        const char* printers_json = orca_slicer_get_printers_list(g_orca_slicer_instance);
        return env->NewStringUTF(printers_json);
    }

    JNIEXPORT jstring JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeGetMaterialsList(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return env->NewStringUTF("[]");
        }
        
        const char* materials_json = orca_slicer_get_materials_list(g_orca_slicer_instance);
        return env->NewStringUTF(materials_json);
    }

    JNIEXPORT jstring JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeGetPrintSettingsList(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return env->NewStringUTF("[]");
        }
        
        const char* settings_json = orca_slicer_get_print_settings_list(g_orca_slicer_instance);
        return env->NewStringUTF(settings_json);
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeSelectPrinter(JNIEnv* env, jobject thiz, jstring printer_name) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        const char* name = env->GetStringUTFChars(printer_name, nullptr);
        bool result = orca_slicer_select_printer(g_orca_slicer_instance, name);
        env->ReleaseStringUTFChars(printer_name, name);
        
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeSelectMaterial(JNIEnv* env, jobject thiz, jstring material_name) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        const char* name = env->GetStringUTFChars(material_name, nullptr);
        bool result = orca_slicer_select_material(g_orca_slicer_instance, name);
        env->ReleaseStringUTFChars(material_name, name);
        
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeSelectPrintSettings(JNIEnv* env, jobject thiz, jstring settings_name) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        const char* name = env->GetStringUTFChars(settings_name, nullptr);
        bool result = orca_slicer_select_print_settings(g_orca_slicer_instance, name);
        env->ReleaseStringUTFChars(settings_name, name);
        
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jstring JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeGetModelInfo(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return env->NewStringUTF("[]");
        }
        
        const char* model_info = orca_slicer_get_model_info(g_orca_slicer_instance);
        return env->NewStringUTF(model_info);
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeRotateModel(JNIEnv* env, jobject thiz, jint object_id, jfloat angle, jint axis) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        bool result = orca_slicer_rotate_model(g_orca_slicer_instance, object_id, angle, axis);
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeScaleModel(JNIEnv* env, jobject thiz, jint object_id, jfloat scale) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        bool result = orca_slicer_scale_model(g_orca_slicer_instance, object_id, scale);
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeTranslateModel(JNIEnv* env, jobject thiz, jint object_id, jfloat x, jfloat y, jfloat z) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        bool result = orca_slicer_translate_model(g_orca_slicer_instance, object_id, x, y, z);
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeDeleteModel(JNIEnv* env, jobject thiz, jint object_id) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        bool result = orca_slicer_delete_model(g_orca_slicer_instance, object_id);
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jstring JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeGetSlicePreviewInfo(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return env->NewStringUTF("{}");
        }
        
        const char* preview_info = orca_slicer_get_slice_preview_info(g_orca_slicer_instance);
        return env->NewStringUTF(preview_info);
    }

    JNIEXPORT jstring JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeGetGCodePreviewInfo(JNIEnv* env, jobject thiz) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return env->NewStringUTF("{}");
        }
        
        const char* preview_info = orca_slicer_get_gcode_preview_info(g_orca_slicer_instance);
        return env->NewStringUTF(preview_info);
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_NativeInterface_nativeExportGCodeToFile(JNIEnv* env, jobject thiz, jstring file_path) {
        if (!g_orca_slicer_instance) {
            LOGE("OrcaSlicer instance is null");
            return JNI_FALSE;
        }
        
        const char* path = env->GetStringUTFChars(file_path, nullptr);
        bool result = orca_slicer_export_gcode_to_file(g_orca_slicer_instance, path);
        env->ReleaseStringUTFChars(file_path, path);
        
        return result ? JNI_TRUE : JNI_FALSE;
    }
}