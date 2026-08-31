# Native code resolves these through jni by their exact class and method name.
-keep class com.xplpc.core.XPLPC { *; }
-keep class com.xplpc.proxy.PlatformProxy { *; }
-keep class com.xplpc.helper.ByteArrayHelper { *; }
-keep class com.xplpc.helper.ByteBufferHelper { *; }

# The serializer reads these field names to build the wire format, so renaming them breaks every call.
-keepclassmembers class com.xplpc.message.Param { <fields>; }
-keepclassmembers class com.xplpc.serializer.JsonSerializer$* { <fields>; }
