package com.xplpc.serializer

import com.google.gson.Gson
import com.google.gson.JsonElement
import com.google.gson.JsonObject
import com.google.gson.JsonParseException
import com.google.gson.TypeAdapter
import com.google.gson.TypeAdapterFactory
import com.google.gson.reflect.TypeToken
import com.google.gson.stream.JsonReader
import com.google.gson.stream.JsonToken
import com.google.gson.stream.JsonWriter
import java.lang.reflect.GenericArrayType
import java.lang.reflect.Modifier
import java.lang.reflect.ParameterizedType
import java.lang.reflect.Type
import java.lang.reflect.TypeVariable
import java.lang.reflect.WildcardType
import kotlin.reflect.KClass
import kotlin.reflect.KFunction
import kotlin.reflect.KMutableProperty1
import kotlin.reflect.KParameter
import kotlin.reflect.KType
import kotlin.reflect.full.memberProperties
import kotlin.reflect.full.primaryConstructor
import kotlin.reflect.jvm.isAccessible
import kotlin.reflect.jvm.javaField
import kotlin.reflect.jvm.javaType

// Fills a kotlin class through its primary constructor, so a field declared never null refuses a value without it and a declared default fills a field the wire left out.
internal class KotlinClassAdapterFactory : TypeAdapterFactory {
    override fun <T> create(
        gson: Gson,
        type: TypeToken<T>
    ): TypeAdapter<T>? {
        val rawType = type.rawType

        if (!isConstructible(rawType)) {
            return null
        }

        return KotlinClassAdapter(gson, gson.getDelegateAdapter(this, type), type)
    }

    private fun isConstructible(rawType: Class<*>): Boolean {
        val concrete = !rawType.isEnum && !rawType.isInterface && !Modifier.isAbstract(rawType.modifiers)

        return concrete && rawType.isAnnotationPresent(Metadata::class.java)
    }
}

// Writing goes through gson's own adapter, and kotlin reflection is reached only once a value is read, so a class shrunk for release still writes.
private class KotlinClassAdapter<T>(
    private val gson: Gson,
    private val delegate: TypeAdapter<T>,
    private val type: TypeToken<T>
) : TypeAdapter<T>() {
    private val kotlinClass: KClass<*> by lazy { type.rawType.kotlin }

    private val constructor: KFunction<*>? by lazy { kotlinClass.primaryConstructor?.also { it.isAccessible = true } }

    private val bodyProperties by lazy {
        val parameters = constructor?.parameters.orEmpty()

        kotlinClass.memberProperties
            .filterIsInstance<KMutableProperty1<Any, Any?>>()
            .filter { property -> parameters.none { it.name == property.name } }
            .filter { property -> property.javaField?.let { !Modifier.isTransient(it.modifiers) } ?: false }
            .onEach { it.isAccessible = true }
    }

    override fun write(
        out: JsonWriter,
        value: T?
    ) = delegate.write(out, value)

    override fun read(reader: JsonReader): T? {
        if (reader.peek() == JsonToken.NULL) {
            reader.nextNull()
            return null
        }

        val element = gson.getAdapter(JsonElement::class.java).read(reader)

        if (!element.isJsonObject) {
            throw JsonParseException("The value is not an object")
        }

        // A class with no primary constructor has no declared shape to fill, so gson fills it field by field.
        val primary = constructor ?: return delegate.fromJsonTree(element)
        val fields = element.asJsonObject
        val instance = primary.callBy(argumentsOf(primary, fields))

        for (property in bodyProperties) {
            val value = fields[property.name] ?: continue

            property.set(instance as Any, readField(value, property.returnType, property.name))
        }

        @Suppress("UNCHECKED_CAST")
        return instance as T
    }

    private fun argumentsOf(
        primary: KFunction<*>,
        fields: JsonObject
    ): Map<KParameter, Any?> {
        val arguments = HashMap<KParameter, Any?>()

        for (parameter in primary.parameters) {
            val name = parameter.name ?: throw JsonParseException("A constructor parameter has no name")
            val value = fields[name]

            // A field the wire left out takes the default the class declares, so the constructor is called without it.
            if (value == null && parameter.isOptional) {
                continue
            }

            arguments[parameter] = if (value != null) readField(value, parameter.type, name) else absent(parameter, name)
        }

        return arguments
    }

    private fun absent(
        parameter: KParameter,
        name: String
    ): Any? {
        if (!parameter.type.isMarkedNullable) {
            throw JsonParseException("Field $name is missing")
        }

        return null
    }

    private fun readField(
        value: JsonElement,
        declared: KType,
        name: String
    ): Any? {
        val decoded = if (value.isJsonNull) null else gson.fromJson<Any?>(value, resolve(declared.javaType))

        if (!Nullability.holds(decoded, declared)) {
            throw JsonParseException("Field $name carries null where its type holds none")
        }

        return decoded
    }

    // A field declared through a type parameter of the class reads as the type argument the caller asked for.
    private fun resolve(declared: Type): Type {
        val requested = type.type as? ParameterizedType ?: return declared
        val parameters = type.rawType.typeParameters

        return substitute(declared) { variable ->
            parameters.indexOfFirst { it.name == variable.name }.takeIf { it >= 0 }?.let { requested.actualTypeArguments[it] }
        }
    }

    private fun substitute(
        declared: Type,
        argumentOf: (TypeVariable<*>) -> Type?
    ): Type =
        when (declared) {
            is TypeVariable<*> -> {
                argumentOf(declared) ?: declared
            }

            is WildcardType -> {
                substitute(declared.upperBounds.first(), argumentOf)
            }

            is GenericArrayType -> {
                TypeToken.getArray(substitute(declared.genericComponentType, argumentOf)).type
            }

            is ParameterizedType -> {
                val raw = declared.rawType as Class<*>
                val arguments = declared.actualTypeArguments.map { substitute(it, argumentOf) }

                TypeToken.getParameterized(raw, *arguments.toTypedArray()).type
            }

            else -> {
                declared
            }
        }
}
