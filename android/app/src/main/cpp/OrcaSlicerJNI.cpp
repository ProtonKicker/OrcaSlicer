#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#define LOG_TAG "OrcaSlicerJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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
}

// JNI方法实现
extern "C" {

    JNIEXPORT jlong JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeInit(JNIEnv* env, jobject thiz, jstring data_path) {
        const char* path = env->GetStringUTFChars(data_path, nullptr);
        void* instance = orca_slicer_init(path);
        env->ReleaseStringUTFChars(data_path, path);
        return reinterpret_cast<jlong>(instance);
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jlong native_ptr, jobject surface) {
        if (native_ptr == 0) return;
        
        // 获取ANativeWindow
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            LOGE("Could not get native window from surface");
            return;
        }
        
        // 设置初始尺寸（实际尺寸将在surfaceChanged中更新）
        orca_slicer_set_surface(
            reinterpret_cast<void*>(native_ptr),
            window,
            ANativeWindow_getWidth(window),
            ANativeWindow_getHeight(window)
        );
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jlong native_ptr, jobject surface, jint width, jint height) {
        if (native_ptr == 0) return;
        
        // 获取ANativeWindow
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            LOGE("Could not get native window from surface");
            return;
        }
        
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
        if (native_ptr == 0) return;
        
        // 通知表面已销毁
        orca_slicer_surface_destroyed(reinterpret_cast<void*>(native_ptr));
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnResume(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) return;
        
        // 通知应用程序已恢复
        orca_slicer_resume(reinterpret_cast<void*>(native_ptr));
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeOnPause(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) return;
        
        // 通知应用程序已暂停
        orca_slicer_pause(reinterpret_cast<void*>(native_ptr));
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeDestroy(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) return;
        
        // 销毁应用程序实例
        orca_slicer_destroy(reinterpret_cast<void*>(native_ptr));
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeImportModel(JNIEnv* env, jobject thiz, jlong native_ptr, jstring model_path) {
        if (native_ptr == 0) return;
        
        // 导入模型
        const char* path = env->GetStringUTFChars(model_path, nullptr);
        bool success = orca_slicer_import_model(reinterpret_cast<void*>(native_ptr), path);
        env->ReleaseStringUTFChars(model_path, path);
        
        // 如果导入失败，可以通过JNI回调通知Java层
        if (!success) {
            // 获取MainActivity类
            jclass mainActivityClass = env->GetObjectClass(thiz);
            
            // 获取onModelImportFailed方法ID
            jmethodID methodId = env->GetMethodID(mainActivityClass, "onModelImportFailed", "()V");
            
            // 调用方法
            if (methodId) {
                env->CallVoidMethod(thiz, methodId);
            }
        }
    }

    JNIEXPORT void JNICALL
    Java_com_softfever3d_orcaslicer_MainActivity_nativeStartSlicing(JNIEnv* env, jobject thiz, jlong native_ptr) {
        if (native_ptr == 0) return;
        
        // 开始切片
        bool success = orca_slicer_start_slicing(reinterpret_cast<void*>(native_ptr));
        
        // 如果切片失败，立即通知Java层
        if (!success) {
            // 获取MainActivity类
            jclass mainActivityClass = env->GetObjectClass(thiz);
            
            // 获取onSlicingFinished方法ID
            jmethodID methodId = env->GetMethodID(mainActivityClass, "onSlicingFinished", "(Z)V");
            
            // 调用方法，传递false表示失败
            if (methodId) {
                env->CallVoidMethod(thiz, methodId, JNI_FALSE);
            }
        }
        
        // 注意：如果切片成功启动，将在切片完成后通过另一个线程调用onSlicingFinished
    }
}