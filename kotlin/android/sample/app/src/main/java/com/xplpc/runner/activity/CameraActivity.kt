package com.xplpc.runner.activity

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.core.UseCaseGroup
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.xplpc.client.Client
import com.xplpc.helper.ByteArrayHelper
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.runner.R
import com.xplpc.runner.databinding.ActivityCameraBinding
import com.xplpc.type.DataView
import com.xplpc.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers

class CameraActivity :
    AppCompatActivity(),
    ImageAnalysis.Analyzer,
    CoroutineScope {
    override val coroutineContext = Dispatchers.Main

    private lateinit var binding: ActivityCameraBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityCameraBinding.inflate(layoutInflater)
        setContentView(binding.root)

        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        setupCamera()
    }

    private fun setupCamera() {
        val future = ProcessCameraProvider.getInstance(applicationContext)

        future.addListener({
            try {
                startCameraX(future.get())
            } catch (e: Exception) {
                Log.e("[CameraActivity : setupCamera] The camera provider is not available")
                Log.d("[CameraActivity : setupCamera] The camera provider is not available: ${e.message}")
            }
        }, ContextCompat.getMainExecutor(this))
    }

    @SuppressLint("RestrictedApi")
    private fun startCameraX(cameraProvider: ProcessCameraProvider) {
        val preview = Preview.Builder().build()
        preview.setSurfaceProvider(binding.vPreview.surfaceProvider)

        val cameraSelector = CameraSelector.Builder().build()

        val imageAnalysis =
            ImageAnalysis
                .Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()

        imageAnalysis.setAnalyzer(ContextCompat.getMainExecutor(this), this)

        // The view port only exists once the preview has been laid out, and the provider can resolve before that.
        val viewPort = binding.vPreview.viewPort

        if (viewPort == null) {
            binding.vPreview.post { startCameraX(cameraProvider) }
            return
        }

        val useCaseGroup =
            UseCaseGroup
                .Builder()
                .addUseCase(preview)
                .addUseCase(imageAnalysis)
                .setViewPort(viewPort)
                .build()

        cameraProvider.unbindAll()

        cameraProvider.bindToLifecycle(
            (this as LifecycleOwner),
            cameraSelector,
            useCaseGroup
        )
    }

    override fun analyze(image: ImageProxy) {
        val bitmap = binding.vPreview.bitmap
        image.close()

        if (bitmap == null) {
            return
        }

        val dataView = DataView.createFromByteArray(bitmapToRgba(bitmap))

        val startTime = System.currentTimeMillis()

        val request =
            Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

        Client.call<String>(request) { response ->
            if (response == null) {
                return@call
            }

            val elapsedTime = System.currentTimeMillis() - startTime
            val duration = (elapsedTime / 1000f)

            val processedPreview =
                bitmapFromRgba(bitmap.width, bitmap.height, ByteArrayHelper.createFromDataView(dataView))

            runOnUiThread {
                binding.vProcessedPreview.setImageBitmap(processedPreview)
                binding.tvOverlay.text =
                    getString(
                        R.string.process_info, duration, (bitmap.byteCount / 1024)
                    )
            }
        }
    }

    private fun bitmapToRgba(bitmap: Bitmap): ByteArray {
        require(bitmap.config == Bitmap.Config.ARGB_8888) { "Bitmap must be in ARGB_8888 format" }

        val pixels = IntArray(bitmap.width * bitmap.height)
        val bytes = ByteArray(pixels.size * 4)
        bitmap.getPixels(pixels, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)
        var i = 0

        for (pixel in pixels) {
            val pixelA = pixel shr 24 and 0xff
            val pixelR = pixel shr 16 and 0xff
            val pixelG = pixel shr 8 and 0xff
            val pixelB = pixel and 0xff

            bytes[i++] = pixelR.toByte()
            bytes[i++] = pixelG.toByte()
            bytes[i++] = pixelB.toByte()
            bytes[i++] = pixelA.toByte()
        }

        return bytes
    }

    private fun bitmapFromRgba(
        width: Int,
        height: Int,
        bytes: ByteArray
    ): Bitmap {
        val pixels = IntArray(bytes.size / 4)
        var j = 0

        for (i in pixels.indices) {
            val pixelR = bytes[j++].toInt() and 0xff
            val pixelG = bytes[j++].toInt() and 0xff
            val pixelB = bytes[j++].toInt() and 0xff
            val pixelA = bytes[j++].toInt() and 0xff
            val pixel = pixelA shl 24 or (pixelR shl 16) or (pixelG shl 8) or pixelB
            pixels[i] = pixel
        }

        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        bitmap.setPixels(pixels, 0, width, 0, 0, width, height)
        return bitmap
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    override fun onNavigateUp(): Boolean {
        finish()
        return true
    }
}
