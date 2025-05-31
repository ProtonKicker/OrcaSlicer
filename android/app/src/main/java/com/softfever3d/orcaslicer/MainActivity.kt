package com.softfever3d.orcaslicer

import android.content.Intent
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.progressindicator.CircularProgressIndicator
import com.google.android.material.snackbar.Snackbar
import com.softfever3d.orcaslicer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var binding: ActivityMainBinding
    private var nativePtr: Long = 0
    private var isSlicing = false

    // 启动导入活动的结果处理器
    private val importLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == RESULT_OK) {
            result.data?.let { data ->
                val modelPath = data.getStringExtra("model_path")
                val modelName = data.getStringExtra("model_name")
                
                if (modelPath != null) {
                    // 调用本地方法导入模型
                    importModel(modelPath)
                    
                    // 显示成功消息
                    Snackbar.make(
                        binding.root,
                        "已导入模型: $modelName",
                        Snackbar.LENGTH_SHORT
                    ).show()
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        setSupportActionBar(binding.toolbar)
        
        // 初始化OpenGL渲染表面
        binding.surfaceView.holder.addCallback(this)
        
        // 设置按钮点击事件
        setupButtons()
        
        // 初始化本地OrcaSlicer
        initializeNative()
    }

    private fun setupButtons() {
        // 导入按钮
        binding.btnImport.setOnClickListener {
            val intent = Intent(this, ImportActivity::class.java)
            importLauncher.launch(intent)
        }
        
        // 切片按钮
        binding.btnSlice.setOnClickListener {
            if (!isSlicing) {
                isSlicing = true
                binding.btnSlice.isEnabled = false
                
                // 显示进度条
                binding.progressBar.show()
                
                // 调用本地方法开始切片
                startSlicing()
            }
        }
        
        // 设置按钮
        binding.btnSettings.setOnClickListener {
            val intent = Intent(this, SettingsActivity::class.java)
            startActivity(intent)
        }
    }

    private fun initializeNative() {
        // 获取应用程序数据目录
        val dataPath = applicationContext.filesDir.absolutePath
        
        // 初始化本地OrcaSlicer
        nativePtr = NativeInterface.initialize(dataPath)
        
        if (nativePtr == 0L) {
            // 初始化失败
            Toast.makeText(
                this,
                "初始化OrcaSlicer失败",
                Toast.LENGTH_LONG
            ).show()
            finish()
            return
        }
        
        // 应用设置到本地代码
        SettingsActivity.applySettingsToNative(this)
    }

    // SurfaceHolder.Callback实现
    override fun surfaceCreated(holder: SurfaceHolder) {
        // 通知本地代码表面已创建
        NativeInterface.onSurfaceCreated(nativePtr, holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // 通知本地代码表面已更改
        NativeInterface.onSurfaceChanged(nativePtr, holder.surface, width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        // 通知本地代码表面已销毁
        NativeInterface.onSurfaceDestroyed(nativePtr)
    }

    override fun onResume() {
        super.onResume()
        // 通知本地代码应用程序已恢复
        NativeInterface.onResume(nativePtr)
    }

    override fun onPause() {
        super.onPause()
        // 通知本地代码应用程序已暂停
        NativeInterface.onPause(nativePtr)
    }

    override fun onDestroy() {
        super.onDestroy()
        // 释放本地资源
        NativeInterface.destroy(nativePtr)
        nativePtr = 0
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_settings -> {
                val intent = Intent(this, SettingsActivity::class.java)
                startActivity(intent)
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    // 导入模型的方法
    private fun importModel(modelPath: String) {
        // 调用本地接口导入模型
        val success = NativeInterface.importModel(nativePtr, modelPath)
        
        if (!success) {
            Snackbar.make(
                binding.root,
                getString(R.string.error_loading_model),
                Snackbar.LENGTH_LONG
            ).show()
        }
    }

    // 开始切片的方法
    private fun startSlicing() {
        // 调用本地接口开始切片
        val success = NativeInterface.startSlicing(nativePtr)
        
        if (!success) {
            // 如果启动切片失败，恢复UI状态
            isSlicing = false
            binding.btnSlice.isEnabled = true
            binding.progressBar.hide()
            
            Snackbar.make(
                binding.root,
                getString(R.string.error_slicing),
                Snackbar.LENGTH_LONG
            ).show()
        }
    }

    // 切片完成的回调方法（由本地代码调用）
    @Suppress("unused")
    private fun onSlicingFinished(success: Boolean) {
        runOnUiThread {
            isSlicing = false
            binding.btnSlice.isEnabled = true
            binding.progressBar.hide()
            
            if (success) {
                Snackbar.make(
                    binding.root,
                    "切片完成",
                    Snackbar.LENGTH_SHORT
                ).show()
            } else {
                Snackbar.make(
                    binding.root,
                    "切片失败",
                    Snackbar.LENGTH_LONG
                ).show()
            }
        }
    }

    companion object {
        // 空的companion object，本地方法已移至NativeInterface类
    }
}