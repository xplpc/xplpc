package com.xplpc.runner.activity

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
import android.util.Size
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.core.UseCaseGroup
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.google.common.util.concurrent.ListenableFuture
import com.xplpc.client.RemoteClient
import com.xplpc.message.Param
import com.xplpc.message.Request
import com.xplpc.runner.R
import com.xplpc.runner.databinding.ActivityCameraBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers

class CameraActivity : AppCompatActivity(), ImageAnalysis.Analyzer, CoroutineScope {
    override val coroutineContext = Dispatchers.Main

    private lateinit var binding: ActivityCameraBinding

    private var cameraProviderFuture: ListenableFuture<ProcessCameraProvider>? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityCameraBinding.inflate(layoutInflater)
        setContentView(binding.root)

        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        setupCamera()
    }

    private fun setupCamera() {
        cameraProviderFuture = ProcessCameraProvider.getInstance(applicationContext)
        cameraProviderFuture?.addListener({
            try {
                val cameraProvider = cameraProviderFuture!!.get()
                startCameraX(cameraProvider)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, ContextCompat.getMainExecutor(this))
    }

    @SuppressLint("RestrictedApi")
    private fun startCameraX(cameraProvider: ProcessCameraProvider) {
        // preview
        val preview = Preview.Builder().build()
        preview.setSurfaceProvider(binding.vPreview.surfaceProvider)

        // prepare
        val cameraSelector = CameraSelector.Builder().build()

        // image analysis use case
        val imageAnalysis = ImageAnalysis.Builder()
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .setTargetResolution(Size(240, 320))
            .build()

        imageAnalysis.setAnalyzer(ContextCompat.getMainExecutor(this), this)

        // use group
        val useCaseGroup = UseCaseGroup.Builder()
            .addUseCase(preview)
            .addUseCase(imageAnalysis)
            .setViewPort(binding.vPreview.viewPort!!)
            .build()

        // bind to lifecycle
        cameraProvider.unbindAll()

        cameraProvider.bindToLifecycle(
            (this as LifecycleOwner),
            cameraSelector,
            useCaseGroup
        )
    }

    override fun analyze(image: ImageProxy) {
        // process image for current frame
        var bitmap = binding.vPreview.bitmap
        image.close()

        if (bitmap == null) {
            return
        }

        bitmap = resizeBitmap(bitmap)

        // convert
        val startTime = System.currentTimeMillis()

        val request = Request(
            "sample.image.grayscale",
            Param("image", bitmapToRgba(bitmap)),
            Param("width", bitmap.width),
            Param("height", bitmap.height)
        )

        RemoteClient.call<ByteArray>(request) { response ->
            if (response == null) {
                return@call
            }

            val elapsedTime = System.currentTimeMillis() - startTime
            val duration = (elapsedTime / 1000f)

            val processedPreview = bitmapFromRgba(bitmap.width, bitmap.height, response)

            runOnUiThread {
                binding.vProcessedPreview.setImageBitmap(processedPreview)
                binding.tvOverlay.text =
                    getString(R.string.process_info, duration, (bitmap.byteCount / 1024))
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
            // get components assuming is argb
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

    private fun bitmapFromRgba(width: Int, height: Int, bytes: ByteArray): Bitmap? {
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

    private fun resizeBitmap(bitmap: Bitmap, width: Int = 100): Bitmap {
        val aspectRatio = bitmap.height.toFloat() / bitmap.width.toFloat()
        val height = (width * aspectRatio).toInt()
        return Bitmap.createScaledBitmap(bitmap, width, height, false)
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
