package com.margelo.nitro.nitroframeprocessor

import android.content.Context
import android.system.Os
import android.util.Log
import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfoProvider
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream
import java.util.concurrent.atomic.AtomicBoolean

class NitroFrameProcessorPackage : BaseReactPackage() {
    override fun getModule(name: String, reactContext: ReactApplicationContext): NativeModule? {
        ensureContextVisionFilesReady(reactContext)
        return null
    }

    override fun getReactModuleInfoProvider(): ReactModuleInfoProvider {
        return ReactModuleInfoProvider { HashMap() }
    }

    companion object {
        private const val TAG = "NitroFrameProcessor"
        private val isContextVisionSetupDone = AtomicBoolean(false)

        init {
            System.loadLibrary("nitroframeprocessor")
            tryInitializeSetupFromApplicationContext()
        }

        private fun tryInitializeSetupFromApplicationContext() {
            try {
                val activityThreadClass = Class.forName("android.app.ActivityThread")
                val currentApplicationMethod = activityThreadClass.getMethod("currentApplication")
                val application = currentApplicationMethod.invoke(null) as? Context
                if (application != null) {
                    ensureContextVisionFilesReady(application)
                } else {
                    Log.w(TAG, "Unable to get Application context for eager ContextVision setup.")
                }
            } catch (e: Throwable) {
                Log.w(TAG, "Eager ContextVision setup unavailable, will retry via React context.", e)
            }
        }

        private fun ensureContextVisionFilesReady(context: Context) {
            if (isContextVisionSetupDone.get()) return
            synchronized(isContextVisionSetupDone) {
                if (isContextVisionSetupDone.get()) return
                try {
                    val targetDir = context.getExternalFilesDir(null) ?: context.filesDir
                    Os.setenv("COV_LICENSE_LOCATION", targetDir.absolutePath, true)
                    copyContextVisionAssets(context, targetDir)
                } catch (e: Throwable) {
                    Log.e(TAG, "ContextVision setup failed", e)
                } finally {
                    isContextVisionSetupDone.set(true)
                }
            }
        }

        private fun copyContextVisionAssets(context: Context, targetDir: File) {
            val foldersToInspect = listOf("", "context_vision")
            val allowedExtensions = listOf(
                ".cov",
                ".us2d4",
                ".us2d5",
                ".us2d6",
                ".us2d7",
                ".us2d9"
            )

            foldersToInspect.forEach { folder ->
                val assets = context.assets.list(folder) ?: return@forEach
                assets.forEach { fileName ->
                    if (!allowedExtensions.any { fileName.endsWith(it, ignoreCase = true) }) return@forEach
                    val assetPath = if (folder.isEmpty()) fileName else "$folder/$fileName"
                    copyAsset(context, assetPath, File(targetDir, fileName))
                }
            }
        }

        private fun copyAsset(context: Context, assetPath: String, destination: File) {
            try {
                context.assets.open(assetPath).use { input ->
                    FileOutputStream(destination, false).use { output ->
                        copyStream(input, output)
                        output.fd.sync()
                    }
                }
            } catch (e: IOException) {
                Log.e(TAG, "Failed to copy asset: $assetPath", e)
            }
        }

        private fun copyStream(input: InputStream, output: FileOutputStream) {
            val buffer = ByteArray(4096)
            while (true) {
                val bytesRead = input.read(buffer)
                if (bytesRead < 0) break
                output.write(buffer, 0, bytesRead)
            }
        }
    }
}
