package com.softfever3d.orcaslicer

import android.view.Surface

/**
 * 本地接口类，用于处理Java/Kotlin代码与C++本地代码之间的交互。
 * 所有与本地代码的交互都应该通过这个类进行，以保持代码的一致性和可维护性。
 */
object NativeInterface {
    
    /**
     * 初始化本地OrcaSlicer
     * @param dataPath 应用程序数据目录路径
     * @return 本地OrcaSlicer实例的指针
     */
    fun initialize(dataPath: String): Long {
        return nativeInit(dataPath)
    }
    
    /**
     * 设置线程数量
     * @param threadCount 要使用的线程数量
     */
    fun setThreadCount(threadCount: Int) {
        nativeSetThreadCount(threadCount)
    }
    
    /**
     * 设置专家模式
     * @param enabled 是否启用专家模式
     */
    fun setExpertMode(enabled: Boolean) {
        nativeSetExpertMode(enabled)
    }
    
    /**
     * 设置语言
     * @param languageCode 语言代码，例如"en"、"zh_CN"等
     */
    fun setLanguage(languageCode: String) {
        nativeSetLanguage(languageCode)
    }
    
    /**
     * 导入模型
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param modelPath 模型文件路径
     * @return 导入是否成功
     */
    fun importModel(nativePtr: Long, modelPath: String): Boolean {
        return nativeImportModel(nativePtr, modelPath)
    }
    
    /**
     * 开始切片
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return 切片是否成功启动
     */
    fun startSlicing(nativePtr: Long): Boolean {
        return nativeStartSlicing(nativePtr)
    }
    
    /**
     * 应用所有设置
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param settings 设置项的Map
     */
    fun applySettings(nativePtr: Long, settings: Map<String, Any>) {
        nativeApplySettings(nativePtr, settings)
    }
    
    /**
     * 设置Surface
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param surface Surface对象
     */
    fun onSurfaceCreated(nativePtr: Long, surface: Surface) {
        nativeOnSurfaceCreated(nativePtr, surface)
    }
    
    /**
     * Surface大小改变
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param surface Surface对象
     * @param width 宽度
     * @param height 高度
     */
    fun onSurfaceChanged(nativePtr: Long, surface: Surface, width: Int, height: Int) {
        nativeOnSurfaceChanged(nativePtr, surface, width, height)
    }
    
    /**
     * Surface销毁
     * @param nativePtr 本地OrcaSlicer实例的指针
     */
    fun onSurfaceDestroyed(nativePtr: Long) {
        nativeOnSurfaceDestroyed(nativePtr)
    }
    
    /**
     * 应用恢复
     * @param nativePtr 本地OrcaSlicer实例的指针
     */
    fun onResume(nativePtr: Long) {
        nativeOnResume(nativePtr)
    }
    
    /**
     * 应用暂停
     * @param nativePtr 本地OrcaSlicer实例的指针
     */
    fun onPause(nativePtr: Long) {
        nativeOnPause(nativePtr)
    }
    
    /**
     * 销毁本地OrcaSlicer实例
     * @param nativePtr 本地OrcaSlicer实例的指针
     */
    fun destroy(nativePtr: Long) {
        nativeDestroy(nativePtr)
    }
    
    // 本地方法声明
    private external fun nativeInit(dataPath: String): Long
    private external fun nativeSetThreadCount(threadCount: Int)
    private external fun nativeSetExpertMode(enabled: Boolean)
    private external fun nativeSetLanguage(languageCode: String)
    private external fun nativeImportModel(nativePtr: Long, modelPath: String): Boolean
    private external fun nativeStartSlicing(nativePtr: Long): Boolean
    private external fun nativeApplySettings(nativePtr: Long, settings: Map<String, Any>)
    private external fun nativeOnSurfaceCreated(nativePtr: Long, surface: Surface)
    private external fun nativeOnSurfaceChanged(nativePtr: Long, surface: Surface, width: Int, height: Int)
    private external fun nativeOnSurfaceDestroyed(nativePtr: Long)
    private external fun nativeOnResume(nativePtr: Long)
    private external fun nativeOnPause(nativePtr: Long)
    private external fun nativeDestroy(nativePtr: Long)
    
    // 加载本地库
    init {
        System.loadLibrary("orcaslicer")
    }
}