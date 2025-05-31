#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <string>

#define LOG_TAG "OrcaSlicerJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 保存全局JavaVM指针，用于在其他线程中获取JNIEnv
static JavaVM* g_jvm = nullptr;

// 保存MainActivity的全局引用，用于回调
static jobject g_main_activity_obj = nullptr;

// 在其他线程中调用Java方法的辅助函数
void CallJavaMethod(const char* method_name, const char* signature, ...) {
    if (g_jvm == nullptr || g_main_activity_obj == nullptr) {
        LOGE("Cannot call Java method: JavaVM or MainActivity reference is null");
        return;
    }
    
    JNIEnv* env = nullptr;
    bool attached = false;
    
    // 获取当前线程的JNIEnv
    int getEnvResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvResult == JNI_EDETACHED) {
        // 当前线程未附加到JVM，需要附加
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("Failed to attach current thread to JVM");
            return;
        }
        attached = true;
    } else if (getEnvResult != JNI_OK) {
        LOGE("Failed to get JNIEnv for current thread");
        return;
    }
    
    // 获取MainActivity类
    jclass mainActivityClass = env->GetObjectClass(g_main_activity_obj);
    if (mainActivityClass == nullptr) {
        LOGE("Failed to get MainActivity class");
        if (attached) g_jvm->DetachCurrentThread();
        return;
    }
    
    // 获取方法ID
    jmethodID methodId = env->GetMethodID(mainActivityClass, method_name, signature);
    if (methodId == nullptr) {
        LOGE("Failed to find method %s with signature %s", method_name, signature);
        if (attached) g_jvm->DetachCurrentThread();
        return;
    }
    
    // 调用方法（这里只实现了无参数的方法调用，可以根据需要扩展）
    env->CallVoidMethod(g_main_activity_obj, methodId);
    
    // 如果当前线程是我们附加的，则需要分离
    if (attached) {
        g_jvm->DetachCurrentThread();
    }
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
    
    // 当切片完成时调用此函数，它会通知Java层
    void orca_slicer_notify_slicing_finished(bool success) {
        LOGI("Slicing finished with result: %s", success ? "success" : "failure");
        CallJavaMethod("onSlicingFinished", "(Z)V", success);
    }
}

// 当库被加载时调用，用于保存JavaVM指针
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad called");
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

// JNI方法实现
extern "C" {

    JNIEXPORT jlong JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeInit(JNIEnv* env, jobject thiz, jstring data_path) {
        if (data_path == nullptr) {
            LOGE("data_path is null");
            return 0;
        }
        
        // 保存MainActivity的全局引用，用于后续回调
        if (g_main_activity_obj != nullptr) {
            // 如果已经有引用，先释放它
            env->DeleteGlobalRef(g_main_activity_obj);
        }
        g_main_activity_obj = env->NewGlobalRef(thiz);
        if (g_main_activity_obj == nullptr) {
            LOGE("Failed to create global reference to MainActivity");
            return 0;
        }
        
        const char* path = env->GetStringUTFChars(data_path, nullptr);
        if (path == nullptr) {
            LOGE("Failed to get data path string");
            return 0;
        }
        
        LOGI("Initializing OrcaSlicer with data path: %s", path);
        void* instance = orca_slicer_init(path);
        env->ReleaseStringUTFChars(data_path, path);
        
        if (instance == nullptr) {
            LOGE("Failed to initialize OrcaSlicer");
            env->DeleteGlobalRef(g_main_activity_obj);
            g_main_activity_obj = nullptr;
            return 0;
        }
        
        LOGI("OrcaSlicer initialized successfully");
        return reinterpret_cast<jlong>(instance);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jlong native_ptr, jobject surface) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeOnSurfaceCreated");
            return;
        }
        
        if (surface == nullptr) {
            LOGE("Surface is null in nativeOnSurfaceCreated");
            return;
        }
        
        // 获取ANativeWindow
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            LOGE("Could not get native window from surface");
            return;
        }
        
        // 获取窗口尺寸
        int width = ANativeWindow_getWidth(window);
        int height = ANativeWindow_getHeight(window);
        LOGI("Surface created with size: %dx%d", width, height);
        
        // 设置初始尺寸（实际尺寸将在surfaceChanged中更新）
        orca_slicer_set_surface(
            reinterpret_cast<void*>(native_ptr),
            window,
            width,
            height
        );
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jlong native_ptr, jobject surface, jint width, jint height) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeOnSurfaceChanged");
            return;
        }
        
        if (surface == nullptr) {
            LOGE("Surface is null in nativeOnSurfaceChanged");
            return;
        }
        
        if (width <= 0 || height <= 0) {
            LOGE("Invalid surface dimensions: %dx%d", width, height);
            return;
        }
        
        // 获取ANativeWindow
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            LOGE("Could not get native window from surface");
            return;
        }
        
        LOGI("Surface changed to size: %dx%d", width, height);
        
        // 更新表面尺寸
        orca_slicer_set_surface(
            reinterpret_cast<void*>(native_ptr),
            window,
            width,
            height
        );
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnSurfaceDestroyed(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeOnSurfaceDestroyed");
            return;
        }
        
        LOGI("Surface is being destroyed");
        orca_slicer_surface_destroyed(reinterpret_cast<void*>(native_ptr));
        LOGI("Surface destroyed successfully");
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnResume(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeOnResume");
            return;
        }
        
        LOGI("Resuming OrcaSlicer");
        orca_slicer_resume(reinterpret_cast<void*>(native_ptr));
        LOGI("OrcaSlicer resumed");
    }
    
    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnPause(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeOnPause");
            return;
        }
        
        LOGI("Pausing OrcaSlicer");
        orca_slicer_pause(reinterpret_cast<void*>(native_ptr));
        LOGI("OrcaSlicer paused");
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeDestroy(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeDestroy");
            return;
        }
        
        LOGI("Destroying OrcaSlicer instance");
        orca_slicer_destroy(reinterpret_cast<void*>(native_ptr));
        
        // 释放MainActivity的全局引用
        if (g_main_activity_obj != nullptr) {
            env->DeleteGlobalRef(g_main_activity_obj);
            g_main_activity_obj = nullptr;
            LOGI("Released global reference to MainActivity");
        }
        
        LOGI("OrcaSlicer instance destroyed successfully");
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeImportModel(JNIEnv* env, jobject thiz, jlong native_ptr, jstring model_path) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeImportModel");
            return JNI_FALSE;
        }
        
        if (model_path == nullptr) {
            LOGE("Model path is null");
            return JNI_FALSE;
        }
        
        const char* path = env->GetStringUTFChars(model_path, nullptr);
        if (path == nullptr) {
            LOGE("Failed to get model path string");
            return JNI_FALSE;
        }
        
        LOGI("Importing model from: %s", path);
        bool result = orca_slicer_import_model(reinterpret_cast<void*>(native_ptr), path);
        env->ReleaseStringUTFChars(model_path, path);
        
        if (result) {
            LOGI("Model imported successfully");
        } else {
            LOGE("Failed to import model");
        }
        
        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeStartSlicing(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) {
            LOGE("Invalid native pointer (0) in nativeStartSlicing");
            return JNI_FALSE;
        }
        
        LOGI("Starting slicing process");
        bool result = orca_slicer_start_slicing(reinterpret_cast<void*>(native_ptr));
        
        if (!result) {
            LOGE("Failed to start slicing process");
            return JNI_FALSE;
        }
        
        LOGI("Slicing process started successfully");
        
        // 注意：切片过程是异步的，完成后会通过orca_slicer_notify_slicing_finished函数
        // 调用Java层的onSlicingFinished回调。这个回调是在OrcaSlicerAndroid.cpp中的
        // 切片完成处理程序中调用的。
        
        return JNI_TRUE;
    }
}