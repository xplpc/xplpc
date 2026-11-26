package com.xplpc.serializer

import kotlin.reflect.KClass
import kotlin.reflect.KType
import kotlin.reflect.KTypeParameter
import kotlin.reflect.KTypeProjection
import kotlin.reflect.full.createType
import kotlin.reflect.full.memberProperties
import kotlin.reflect.full.withNullability
import kotlin.reflect.jvm.isAccessible

// Says whether a value read off the wire holds what a kotlin type declares about null, which the jvm erases from every list, map, array and type argument it holds.
@PublishedApi
internal object Nullability {
    fun holds(
        value: Any?,
        type: KType
    ): Boolean {
        if (value == null) {
            return type.isMarkedNullable
        }

        return when (value) {
            is Map<*, *> -> holdsEach(value.values, type, 1)
            is Iterable<*> -> holdsEach(value, type, 0)
            is Array<*> -> holdsEach(value.asIterable(), type, 0)
            else -> holdsTypeArguments(value, type)
        }
    }

    private fun holdsEach(
        values: Iterable<*>,
        type: KType,
        argument: Int
    ): Boolean {
        val declared = type.arguments.getOrNull(argument)?.type ?: return true

        return values.all { holds(it, declared) }
    }

    // A field declared through a type parameter holds what the argument the caller asked for declares, which only the requested type knows.
    private fun holdsTypeArguments(
        value: Any,
        type: KType
    ): Boolean {
        val kotlinClass = type.classifier as? KClass<*> ?: return true

        if (type.arguments.isEmpty() || !value.javaClass.isAnnotationPresent(Metadata::class.java)) {
            return true
        }

        val bindings = kotlinClass.typeParameters.zip(type.arguments).toMap()

        return kotlinClass.memberProperties
            .filter { mentions(it.returnType, bindings.keys) }
            .all { property ->
                property.isAccessible = true
                holds(property.getter.call(value), substitute(property.returnType, bindings))
            }
    }

    private fun mentions(
        type: KType,
        parameters: Set<KTypeParameter>
    ): Boolean {
        val classifier = type.classifier

        if (classifier is KTypeParameter) {
            return classifier in parameters
        }

        return type.arguments.any { argument -> argument.type?.let { mentions(it, parameters) } ?: false }
    }

    private fun substitute(
        type: KType,
        bindings: Map<KTypeParameter, KTypeProjection>
    ): KType {
        val classifier = type.classifier

        if (classifier is KTypeParameter) {
            val bound = bindings[classifier]?.type ?: return type

            return if (type.isMarkedNullable) bound.withNullability(true) else bound
        }

        if (classifier !is KClass<*>) {
            return type
        }

        val arguments =
            type.arguments.map { argument ->
                argument.type?.let { KTypeProjection(argument.variance, substitute(it, bindings)) }
                    ?: argument
            }

        return classifier.createType(arguments, type.isMarkedNullable)
    }
}
