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
    
    /**
     * 获取支持的打印机列表
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return 打印机列表的JSON字符串
     */
    fun getPrintersList(nativePtr: Long): String {
        return nativeGetPrintersList(nativePtr)
    }

    /**
     * 获取支持的材料列表
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return 材料列表的JSON字符串
     */
    fun getMaterialsList(nativePtr: Long): String {
        return nativeGetMaterialsList(nativePtr)
    }

    /**
     * 获取支持的打印设置列表
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return 打印设置列表的JSON字符串
     */
    fun getPrintSettingsList(nativePtr: Long): String {
        return nativeGetPrintSettingsList(nativePtr)
    }

    /**
     * 选择打印机
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param printerName 打印机名称
     * @return 选择是否成功
     */
    fun selectPrinter(nativePtr: Long, printerName: String): Boolean {
        return nativeSelectPrinter(nativePtr, printerName)
    }

    /**
     * 选择材料
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param materialName 材料名称
     * @return 选择是否成功
     */
    fun selectMaterial(nativePtr: Long, materialName: String): Boolean {
        return nativeSelectMaterial(nativePtr, materialName)
    }

    /**
     * 选择打印设置
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param printSettingsName 打印设置名称
     * @return 选择是否成功
     */
    fun selectPrintSettings(nativePtr: Long, printSettingsName: String): Boolean {
        return nativeSelectPrintSettings(nativePtr, printSettingsName)
    }

    /**
     * 获取当前模型信息
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return 模型信息的JSON字符串
     */
    fun getModelInfo(nativePtr: Long): String {
        return nativeGetModelInfo(nativePtr)
    }

    /**
     * 旋转模型
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param objectId 对象ID
     * @param angle 角度（度）
     * @param axis 轴（0=X, 1=Y, 2=Z）
     * @return 操作是否成功
     */
    fun rotateModel(nativePtr: Long, objectId: Int, angle: Float, axis: Int): Boolean {
        return nativeRotateModel(nativePtr, objectId, angle, axis)
    }

    /**
     * 缩放模型
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param objectId 对象ID
     * @param scale 缩放因子
     * @return 操作是否成功
     */
    fun scaleModel(nativePtr: Long, objectId: Int, scale: Float): Boolean {
        return nativeScaleModel(nativePtr, objectId, scale)
    }

    /**
     * 移动模型
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param objectId 对象ID
     * @param x X轴位移
     * @param y Y轴位移
     * @param z Z轴位移
     * @return 操作是否成功
     */
    fun translateModel(nativePtr: Long, objectId: Int, x: Float, y: Float, z: Float): Boolean {
        return nativeTranslateModel(nativePtr, objectId, x, y, z)
    }

    /**
     * 删除模型
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param objectId 对象ID
     * @return 操作是否成功
     */
    fun deleteModel(nativePtr: Long, objectId: Int): Boolean {
        return nativeDeleteModel(nativePtr, objectId)
    }

    /**
     * 获取切片预览信息
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return 切片预览信息的JSON字符串
     */
    fun getSlicePreviewInfo(nativePtr: Long): String {
        return nativeGetSlicePreviewInfo(nativePtr)
    }

    /**
     * 获取G代码预览信息
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @return G代码预览信息的JSON字符串
     */
    fun getGCodePreviewInfo(nativePtr: Long): String {
        return nativeGetGCodePreviewInfo(nativePtr)
    }

    /**
     * 导出G代码到文件
     * @param nativePtr 本地OrcaSlicer实例的指针
     * @param filePath 文件路径
     * @return 导出是否成功
     */
    fun exportGCodeToFile(nativePtr: Long, filePath: String): Boolean {
        return nativeExportGCodeToFile(nativePtr, filePath)
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
    private external fun nativeGetPrintersList(nativePtr: Long): String
    private external fun nativeGetMaterialsList(nativePtr: Long): String
    private external fun nativeGetPrintSettingsList(nativePtr: Long): String
    private external fun nativeSelectPrinter(nativePtr: Long, printerName: String): Boolean
    private external fun nativeSelectMaterial(nativePtr: Long, materialName: String): Boolean
    private external fun nativeSelectPrintSettings(nativePtr: Long, printSettingsName: String): Boolean
    private external fun nativeGetModelInfo(nativePtr: Long): String
    private external fun nativeRotateModel(nativePtr: Long, objectId: Int, angle: Float, axis: Int): Boolean
    private external fun nativeScaleModel(nativePtr: Long, objectId: Int, scale: Float): Boolean
    private external fun nativeTranslateModel(nativePtr: Long, objectId: Int, x: Float, y: Float, z: Float): Boolean
    private external fun nativeDeleteModel(nativePtr: Long, objectId: Int): Boolean
    private external fun nativeGetSlicePreviewInfo(nativePtr: Long): String
    private external fun nativeGetGCodePreviewInfo(nativePtr: Long): String
    private external fun nativeExportGCodeToFile(nativePtr: Long, filePath: String): Boolean
    
    // 加载本地库
    init {
        System.loadLibrary("orcaslicer")
    }
}