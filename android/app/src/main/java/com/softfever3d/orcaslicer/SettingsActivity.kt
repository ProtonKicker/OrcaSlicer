package com.softfever3d.orcaslicer

import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.res.Configuration
import android.net.Uri
import android.os.Bundle
import android.view.MenuItem
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.appcompat.widget.Toolbar
import androidx.preference.EditTextPreference
import androidx.preference.ListPreference
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.PreferenceManager
import androidx.preference.SwitchPreferenceCompat
import com.softfever3d.orcaslicer.BuildConfig
import com.softfever3d.orcaslicer.databinding.ActivitySettingsBinding

class SettingsActivity : AppCompatActivity() {
    private lateinit var binding: ActivitySettingsBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // 根据屏幕方向加载不同的布局
        val orientation = resources.configuration.orientation
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            setContentView(R.layout.activity_settings_landscape)
        } else {
            setContentView(R.layout.activity_settings)
        }
        
        // 设置工具栏
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        
        // 加载设置片段
        supportFragmentManager
            .beginTransaction()
            .replace(R.id.settings_container, SettingsFragment())
            .commit()
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            android.R.id.home -> {
                // 返回按钮处理
                onBackPressed()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    class SettingsFragment : PreferenceFragmentCompat(), SharedPreferences.OnSharedPreferenceChangeListener {
        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            setPreferencesFromResource(R.xml.preferences, rootKey)
            
            // 初始化设置项摘要
            updatePreferenceSummaries()
            
            // 设置版本信息
            val versionPref = findPreference<Preference>("version")
            versionPref?.summary = "v" + BuildConfig.VERSION_NAME
            
            // 设置GitHub链接点击事件
            val githubPref = findPreference<Preference>("github")
            githubPref?.setOnPreferenceClickListener {
                val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/SoftFever/OrcaSlicer"))
                startActivity(intent)
                true
            }
            
            // 设置许可证点击事件
            val licensesPref = findPreference<Preference>("licenses")
            licensesPref?.setOnPreferenceClickListener {
                Toast.makeText(requireContext(), "开源许可证信息", Toast.LENGTH_SHORT).show()
                // TODO: 显示许可证对话框
                true
            }
        }
        
        private fun updatePreferenceSummaries() {
            // 更新语言选择摘要
            val languagePref = findPreference<ListPreference>("language")
            languagePref?.summaryProvider = ListPreference.SimpleSummaryProvider.getInstance()
            
            // 更新线程数量摘要
            val threadCountPref = findPreference<Preference>("thread_count")
            val prefs = PreferenceManager.getDefaultSharedPreferences(requireContext())
            val threadCount = prefs.getInt("thread_count", 4)
            threadCountPref?.summary = "$threadCount 线程"
            
            // 更新默认导出文件夹摘要
            val exportFolderPref = findPreference<EditTextPreference>("export_folder")
            exportFolderPref?.summaryProvider = EditTextPreference.SimpleSummaryProvider.getInstance()
        }
        
        override fun onResume() {
            super.onResume()
            preferenceScreen.sharedPreferences?.registerOnSharedPreferenceChangeListener(this)
        }
        
        override fun onPause() {
            super.onPause()
            preferenceScreen.sharedPreferences?.unregisterOnSharedPreferenceChangeListener(this)
        }
        
        override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String) {
            when (key) {
                "dark_theme" -> {
                    val darkThemeEnabled = sharedPreferences.getBoolean(key, false)
                    if (darkThemeEnabled) {
                        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
                    } else {
                        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
                    }
                }
                "language" -> {
                    // 语言变更需要重启应用
                    val languageCode = sharedPreferences.getString(key, "en") ?: "en"
                    NativeInterface.setLanguage(languageCode)
                    Toast.makeText(requireContext(), "语言设置将在应用重启后生效", Toast.LENGTH_LONG).show()
                }
                "thread_count" -> {
                    val threadCount = sharedPreferences.getInt(key, 4)
                    findPreference<Preference>(key)?.summary = "$threadCount 线程"
                    NativeInterface.setThreadCount(threadCount)
                }
                "expert_mode" -> {
                    val expertMode = sharedPreferences.getBoolean(key, false)
                    NativeInterface.setExpertMode(expertMode)
                }
            }
            
            // 应用设置到本地代码
            applySettingsToNative(requireContext())
        }
    }
    
    companion object {
        /**
         * 将设置应用到本地代码
         * @param context 上下文
         */
        fun applySettingsToNative(context: Context) {
            val prefs = PreferenceManager.getDefaultSharedPreferences(context)
            
            // 收集所有设置项
            val settings = mutableMapOf<String, Any>().apply {
                // 线程数量
                put("thread_count", prefs.getInt("thread_count", 4))
                
                // 专家模式
                put("expert_mode", prefs.getBoolean("expert_mode", false))
                
                // 语言
                put("language", prefs.getString("language", "en") ?: "en")
                
                // 自动居中
                put("auto_center", prefs.getBoolean("auto_center", true))
                
                // 自动排列
                put("auto_arrange", prefs.getBoolean("auto_arrange", true))
                
                // 自动保存
                put("auto_save", prefs.getBoolean("auto_save", false))
                
                // 默认导出文件夹
                put("export_folder", prefs.getString("export_folder", "") ?: "")
            }
            
            // 应用设置到本地代码
            // 注意：这里使用0L作为占位符，实际应用中应该使用有效的nativePtr
            // 在MainActivity中调用时会使用正确的nativePtr
            NativeInterface.applySettings(0L, settings)
        }
    }
}