package com.softfever3d.orcaslicer

import android.app.Activity
import android.content.Intent
import android.content.res.Configuration
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.view.MenuItem
import android.view.View
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.recyclerview.widget.LinearLayoutManager
import com.google.android.material.snackbar.Snackbar
import com.softfever3d.orcaslicer.databinding.ActivityImportBinding
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class ImportActivity : AppCompatActivity() {

    private lateinit var binding: ActivityImportBinding
    private val modelAdapter = ModelAdapter { model -> onModelSelected(model) }
    
    private val openDocumentLauncher = registerForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        uri?.let { handleSelectedFile(it) }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // 根据屏幕方向加载不同的布局
        val orientation = resources.configuration.orientation
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            setContentView(R.layout.activity_import_landscape)
        } else {
            binding = ActivityImportBinding.inflate(layoutInflater)
            setContentView(binding.root)
        }
        
        // 设置工具栏
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.title = getString(R.string.import_model)
        
        // 设置RecyclerView和按钮
        setupRecyclerView()
        setupButtons()
    }
    
    private fun setupRecyclerView() {
        // 获取RecyclerView
        val recyclerView = findViewById<androidx.recyclerview.widget.RecyclerView>(R.id.recyclerModels)
        recyclerView.apply {
            layoutManager = LinearLayoutManager(this@ImportActivity)
            adapter = modelAdapter
        }
        
        // 加载最近导入的模型列表
        loadRecentModels()
    }
    
    private fun setupButtons() {
        // 导入按钮
        val fabImport = findViewById<com.google.android.material.floatingactionbutton.FloatingActionButton>(R.id.fabImport)
        fabImport.setOnClickListener {
            openFilePicker()
        }
    }
    
    private fun openFilePicker() {
        openDocumentLauncher.launch(arrayOf(
            "model/stl",
            "model/obj",
            "model/3mf",
            "model/amf",
            "application/vnd.ms-package.3dmanufacturing-3dmodel+xml",
            "*/*"
        ))
    }
    
    private fun handleSelectedFile(uri: Uri) {
        // 显示加载指示器
        val progressBar = findViewById<android.widget.ProgressBar>(R.id.progressBar)
        progressBar.visibility = View.VISIBLE
        
        try {
            // 获取文件名
            val fileName = getFileName(uri)
            
            // 将文件复制到应用程序的私有存储
            val destinationFile = copyFileToInternal(uri, fileName)
            
            // 创建模型对象
            val model = Model(
                id = System.currentTimeMillis(),
                name = fileName,
                path = destinationFile.absolutePath,
                dateAdded = System.currentTimeMillis()
            )
            
            // 添加到最近模型列表
            addToRecentModels(model)
            
            // 返回结果到主活动
            val resultIntent = Intent().apply {
                putExtra("model_path", model.path)
                putExtra("model_name", model.name)
            }
            setResult(Activity.RESULT_OK, resultIntent)
            finish()
            
        } catch (e: Exception) {
            // 显示错误消息
            val rootView = findViewById<android.view.View>(android.R.id.content)
            Snackbar.make(
                rootView,
                getString(R.string.error_loading_model),
                Snackbar.LENGTH_LONG
            ).show()
            
            e.printStackTrace()
        } finally {
            // 隐藏加载指示器
            progressBar.visibility = View.GONE
        }
    }
    
    private fun getFileName(uri: Uri): String {
        var result = ""
        contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (nameIndex != -1) {
                    result = cursor.getString(nameIndex)
                }
            }
        }
        return result
    }
    
    private fun copyFileToInternal(uri: Uri, fileName: String): File {
        val inputStream = contentResolver.openInputStream(uri)
        val outputFile = File(getExternalFilesDir(null), fileName)
        
        inputStream?.use { input ->
            FileOutputStream(outputFile).use { output ->
                input.copyTo(output)
            }
        }
        
        return outputFile
    }
    
    private fun loadRecentModels() {
        // 这里应该从数据库或文件中加载最近导入的模型列表
        // 这是一个简化的示例，实际实现应该使用Room数据库或其他持久化方法
        val models = listOf<Model>()
        modelAdapter.submitList(models)
        
        // 如果没有最近的模型，显示空视图
        val emptyView = findViewById<android.view.View>(R.id.emptyView)
        if (emptyView != null) {
            emptyView.visibility = if (models.isEmpty()) View.VISIBLE else View.GONE
        }
    }
    
    private fun addToRecentModels(model: Model) {
        // 这里应该将模型添加到数据库或文件中
        // 这是一个简化的示例，实际实现应该使用Room数据库或其他持久化方法
    }
    
    private fun onModelSelected(model: Model) {
        // 返回选中的模型到主活动
        val resultIntent = Intent().apply {
            putExtra("model_path", model.path)
            putExtra("model_name", model.name)
        }
        setResult(Activity.RESULT_OK, resultIntent)
        finish()
    }
    
    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            onBackPressed()
            return true
        }
        return super.onOptionsItemSelected(item)
    }
    
    // 模型数据类
    data class Model(
        val id: Long,
        val name: String,
        val path: String,
        val dateAdded: Long
    )
}