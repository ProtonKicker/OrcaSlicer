package com.softfever3d.orcaslicer

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.softfever3d.orcaslicer.ImportActivity.Model
import com.softfever3d.orcaslicer.databinding.ItemModelBinding
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class ModelAdapter(private val onItemClick: (Model) -> Unit) :
    ListAdapter<Model, ModelAdapter.ModelViewHolder>(ModelDiffCallback()) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ModelViewHolder {
        val binding = ItemModelBinding.inflate(
            LayoutInflater.from(parent.context),
            parent,
            false
        )
        return ModelViewHolder(binding, onItemClick)
    }

    override fun onBindViewHolder(holder: ModelViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    class ModelViewHolder(private val binding: ItemModelBinding, private val onItemClick: (Model) -> Unit) :
        RecyclerView.ViewHolder(binding.root) {

        fun bind(model: Model) {
            binding.apply {
                textModelName.text = model.name
                
                // 设置文件类型图标
                val fileExtension = model.name.substringAfterLast('.', "").lowercase()
                val iconRes = when (fileExtension) {
                    "stl" -> R.drawable.ic_file_stl
                    "obj" -> R.drawable.ic_file_obj
                    "3mf" -> R.drawable.ic_file_3mf
                    "amf" -> R.drawable.ic_file_amf
                    else -> R.drawable.ic_file_generic
                }
                imageModelType.setImageResource(iconRes)
                
                // 格式化日期
                val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())
                textModelDate.text = dateFormat.format(Date(model.dateAdded))
                
                // 设置点击事件
                root.setOnClickListener { onItemClick(model) }
            }
        }
    }

    class ModelDiffCallback : DiffUtil.ItemCallback<Model>() {
        override fun areItemsTheSame(oldItem: Model, newItem: Model): Boolean {
            return oldItem.id == newItem.id
        }

        override fun areContentsTheSame(oldItem: Model, newItem: Model): Boolean {
            return oldItem == newItem
        }
    }
}