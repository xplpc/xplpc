# Native code resolves these through jni by their exact class and method name.
-keep class com.xplpc.layer.Layer { *; }
-keep class com.xplpc.data.PendingCall { *; }

# The serializer reads these field names to build the wire format, so renaming them breaks every call.
-keepclassmembers class com.xplpc.message.Param { <fields>; }
-keepclassmembers class com.xplpc.serializer.JsonSerializer$* { <fields>; }
-keepclassmembers class com.xplpc.type.DataView { long buffer; }

# A kotlin class is filled through its primary constructor, which kotlin reflection finds through the metadata the compiler writes.
-keepattributes RuntimeVisibleAnnotations
-keep class kotlin.Metadata { *; }
