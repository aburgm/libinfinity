/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007-2014 Armin Burgmeier <armin@arbur.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <libinfinity/adopted/inf-adopted-operation.h>
#include <libinfinity/adopted/inf-adopted-split-operation.h>
#include <libinfinity/adopted/inf-adopted-user.h>

/**
 * SECTION:inf-adopted-operation
 * @title: InfAdoptedOperation
 * @short_description: Operation that can be processed by
 * #InfAdoptedAlgorithm.
 * @include: libinfinity/adopted/inf-adopted-operation.h
 * @see_also: #InfAdoptedRequest, #InfAdoptedAlgorithm
 * @stability: Unstable
 *
 * The #InfAdoptedOperation interface must be implemented by operations that
 * are supposed to be used by #InfAdoptedAlgorithm. They basically need to
 * define transformation rules for transformation against other operations.
 **/

static void
inf_adopted_operation_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if(!initialized)
  {
    initialized = TRUE;
  }
}

GType
inf_adopted_concurrency_id_get_type(void)
{
  static GType concurrency_id_type = 0;

  if(!concurrency_id_type)
  {
    static const GEnumValue concurrency_id_type_values[] = {
      {
        INF_ADOPTED_CONCURRENCY_SELF,
        "INF_ADOPTED_CONCURRENCY_SELF",
        "self"
      }, {
        INF_ADOPTED_CONCURRENCY_NONE,
        "INF_ADOPTED_CONCURRENCY_NONE",
        "none"
      }, {
        INF_ADOPTED_CONCURRENCY_OTHER,
        "INF_ADOPTED_CONCURRENCY_OTHER",
        "other"
      }, {
        0,
        NULL,
        NULL
      }
    };

    concurrency_id_type = g_enum_register_static(
      "InfAdoptedConcurrencyId",
      concurrency_id_type_values
    );
  }

  return concurrency_id_type;
}

GType
inf_adopted_operation_flags_get_type(void)
{
  static GType operation_flags_type = 0;

  if(!operation_flags_type)
  {
    static const GFlagsValue operation_flags_type_values[] = {
      {
        INF_ADOPTED_OPERATION_AFFECTS_BUFFER,
        "INF_ADOPTED_OPERATION_AFFECTS_BUFFER",
        "affects_buffer",
      }, {
        INF_ADOPTED_OPERATION_REVERSIBLE,
        "INF_ADOPTED_OPERATION_REVERSIBLE",
        "reversible",
      }, {
        0,
        NULL,
        NULL
      }
    };

    operation_flags_type = g_flags_register_static(
      "InfAdoptedOperationFlags",
      operation_flags_type_values
    );
  }

  return operation_flags_type;
}

GType
inf_adopted_operation_get_type(void)
{
  static GType adopted_operation_type = 0;

  if(!adopted_operation_type)
  {
    static const GTypeInfo adopted_operation_info = {
      sizeof(InfAdoptedOperationIface),         /* class_size */
      inf_adopted_operation_base_init,          /* base_init */
      NULL,                                     /* base_finalize */
      NULL,                                     /* class_init */
      NULL,                                     /* class_finalize */
      NULL,                                     /* class_data */
      0,                                        /* instance_size */
      0,                                        /* n_preallocs */
      NULL,                                     /* instance_init */
      NULL                                      /* value_table */
    };

    adopted_operation_type = g_type_register_static(
      G_TYPE_INTERFACE,
      "InfAdoptedOperation",
      &adopted_operation_info,
      0
    );

    g_type_interface_add_prerequisite(adopted_operation_type, G_TYPE_OBJECT);
  }

  return adopted_operation_type;
}

/**
 * inf_adopted_operation_need_concurrency_id:
 * @operation: The #InfAdoptedOperation to transform.
 * @against: The operation to transform against.
 *
 * This function returns whether transforming @operation against @against
 * is not defined unambiguously. In that case, transformation requires a
 * so-called concurrency ID which determines which of the two operations
 * is transformed.
 *
 * Returns: Whether transformation of @operation against @against requires a
 * concurrency ID to be defined.
 */
gboolean
inf_adopted_operation_need_concurrency_id(InfAdoptedOperation* operation,
                                          InfAdoptedOperation* against)
{
  InfAdoptedOperationIface* iface;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), FALSE);
  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(against), FALSE);
  
  if(INF_ADOPTED_IS_SPLIT_OPERATION(against))
  {
    iface = INF_ADOPTED_OPERATION_GET_IFACE(against);
    g_assert(iface->need_concurrency_id != NULL);

    return iface->need_concurrency_id(against, operation);
  }
  else
  {
    iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);
    g_assert(iface->need_concurrency_id != NULL);

    return iface->need_concurrency_id(operation, against);
  }
}

/**
 * inf_adopted_operation_transform:
 * @operation: The #InfAdoptedOperation to transform.
 * @against: The operation to transform against.
 * @operation_lcs: The operation at a previous state, or %NULL.
 * @against_lcs: The @against operation at a previous state, or %NULL.
 * @concurrency_id: The concurrency ID for the transformation.
 *
 * Performs an inclusion transformation of @operation against @against,
 * meaning that the effect of @against is included in @operation.
 *
 * If inf_adopted_operation_need_concurrency_id() returns %TRUE for @operation
 * and @against, then @operation_lcs and @against_lcs must not be %NULL. In
 * this case they must be the same operations as @operation and @against at
 * the earlierst state to which both of them can be transformed. This
 * information can then be used to resolve any conflicts in the transformation
 * of @operation against @against.
 *
 * The @concurrency_id parameter is used if
 * inf_adopted_operation_need_concurrency_id() returns %TRUE and no conflict
 * resolution can be deduced from @operation_lcs and @against_lcs. In this
 * case @concurrency_id defines a unique way to transform the two operations.
 * Usually, this is derived from the user IDs of the users who issued the two
 * conflicting operations.
 *
 * Return Value: The transformed #InfAdoptedOperation, or %NULL if the
 * transformation failed.
 **/
InfAdoptedOperation*
inf_adopted_operation_transform(InfAdoptedOperation* operation,
                                InfAdoptedOperation* against,
                                InfAdoptedOperation* operation_lcs,
                                InfAdoptedOperation* against_lcs,
                                InfAdoptedConcurrencyId concurrency_id)
{
  InfAdoptedOperationIface* iface;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), NULL);
  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(against), NULL);

  /* Transform against both parts of split operation if we are transforming
   * against split operation. */
  if(INF_ADOPTED_IS_SPLIT_OPERATION(against))
  {
    return inf_adopted_split_operation_transform_other(
      INF_ADOPTED_SPLIT_OPERATION(against),
      operation,
      against_lcs,
      operation_lcs,
      concurrency_id
    );
  }
  else
  {
    iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);
    g_assert(iface->transform != NULL);

    return (*iface->transform)(
      operation,
      against,
      operation_lcs,
      against_lcs,
      concurrency_id
    );
  }
}

/**
 * inf_adopted_operation_copy:
 * @operation: The #InfAdoptedOperation to copy.
 *
 * Returns a copy of @operation.
 *
 * Return Value: A copy of @operation.
 **/
InfAdoptedOperation*
inf_adopted_operation_copy(InfAdoptedOperation* operation)
{
  InfAdoptedOperationIface* iface;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), NULL);

  iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);

  g_assert(iface->copy != NULL);
  return (*iface->copy)(operation);
}

/**
 * inf_adopted_operation_get_flags:
 * @operation: A #InfAdoptedOperation.
 *
 * Returns the flags for @operation.
 *
 * Return Value: #InfAdoptedOperationFlags for @operation.
 **/
InfAdoptedOperationFlags
inf_adopted_operation_get_flags(InfAdoptedOperation* operation)
{
  InfAdoptedOperationIface* iface;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), 0);

  iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);

  if(iface->get_flags != NULL)
    return (*iface->get_flags)(operation);
  else
    return 0;
}

/**
 * inf_adopted_operation_apply:
 * @operation: A #InfAdoptedOperation.
 * @by: A #InfAdoptedUser.
 * @buffer: The #InfBuffer to apply the operation to.
 * @error: Location to store error information, if any, or %NULL.
 *
 * Applies @operation to @buffer. The operation is considered to be applied by
 * user @by. If the operation cannot be applied then @error is set and the
 * function returns %FALSE.
 *
 * Returns: %TRUE on success or %FALSE on error.
 **/
gboolean
inf_adopted_operation_apply(InfAdoptedOperation* operation,
                            InfAdoptedUser* by,
                            InfBuffer* buffer,
                            GError** error)
{
  InfAdoptedOperationIface* iface;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), FALSE);
  g_return_val_if_fail(INF_ADOPTED_IS_USER(by), FALSE);
  g_return_val_if_fail(INF_IS_BUFFER(buffer), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

  iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);

  /* apply must be implemented */
  g_assert(iface->apply != NULL);
  return (*iface->apply)(operation, by, buffer, error);
}

/**
 * inf_adopted_operation_apply_transformed:
 * @operation: A #InfAdoptedOperation.
 * @transformed: A transformed version of @operation.
 * @by: The #InfAdoptedUser applying the operation.
 * @buffer: The #InfBuffer to apply the operation to.
 * @error: Location to store error information, if any, or %NULL.
 *
 * Applies @transformed to @buffer. The operation is considered to be applied
 * by user @by. The operation @transformed must have originated from
 * @operation by transformation with inf_adopted_operation_transform().
 *
 * If @operation is reversible or does not affect the buffer (see
 * #InfAdoptedOperationFlags), this function is equivalent to
 * <code>inf_adopted_operation_apply(transformed, by, buffer)</code>, with
 * the exception of the return value. In that case the return value will be
 * @operation itself, with a reference added.
 *
 * However, if @operation is not reversible and affects the buffer, the
 * function attempts to construct an operation which is identical to
 * @operation, but reversible, and returns it. The function can use
 * information from the transformed operation and the buffer to construct
 * the reversible operation. If a reversible operation cannot be constructed,
 * the function returns an additional reference on @operation, and still
 * applies the transformed operation
 * to the buffer.
 *
 * For example, an operation that deletes text in a text editor would be
 * transmitting only the position and the length of the text to delete over
 * the network. From that information alone, the operation cannot be made
 * reversible. However, when the operation is applied to the buffer, the
 * actual text that is being removed can be restored by looking it up in
 * the buffer, making the operation reversible.
 *
 * Returns: A #InfAdoptedOperation, or %NULL on error. Free with
 * g_object_unref() when no longer needed.
 */
InfAdoptedOperation*
inf_adopted_operation_apply_transformed(InfAdoptedOperation* operation,
                                        InfAdoptedOperation* transformed,
                                        InfAdoptedUser* by,
                                        InfBuffer* buffer,
                                        GError** error)
{
  InfAdoptedOperationIface* iface;
  InfAdoptedOperationFlags flags;
  InfAdoptedOperationFlags check_flags;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), NULL);
  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(transformed), NULL);
  g_return_val_if_fail(INF_ADOPTED_IS_USER(by), NULL);
  g_return_val_if_fail(INF_IS_BUFFER(buffer), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);

  iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);

  flags = inf_adopted_operation_get_flags(operation);

  check_flags = INF_ADOPTED_OPERATION_REVERSIBLE |
                INF_ADOPTED_OPERATION_AFFECTS_BUFFER;
  if( (flags & check_flags) == INF_ADOPTED_OPERATION_AFFECTS_BUFFER)
  {
    if(iface->apply_transformed != NULL)
    {
      return (*iface->apply_transformed)(
        operation,
        transformed,
        by,
        buffer,
        error
      );
    }
    else
    {
      /* apply must be implemented */
      g_assert(iface->apply != NULL);
      if(!(*iface->apply)(transformed, by, buffer, error))
        return NULL;

      g_object_ref(operation);
      return operation;
    }
  }
  else
  {
    /* apply must be implemented */
    g_assert(iface->apply != NULL);
    if(!(*iface->apply)(transformed, by, buffer, error))
      return NULL;

    g_object_ref(operation);
    return operation;
  }
}

/**
 * inf_adopted_operation_is_reversible:
 * @operation: A #InfAdoptedOperation.
 *
 * Returns whether @operation is reversible.
 *
 * Return Value: Whether @operation is reversible.
 **/
gboolean
inf_adopted_operation_is_reversible(InfAdoptedOperation* operation)
{
  InfAdoptedOperationFlags flags;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), FALSE);

  flags = inf_adopted_operation_get_flags(operation);
  if( (flags & INF_ADOPTED_OPERATION_REVERSIBLE) != 0)
    return TRUE;
  else
    return FALSE;
}

/**
 * inf_adopted_operation_revert:
 * @operation: A #InfAdoptedOperation.
 *
 * Returns a new #InfAdoptedOperation that undoes the effect of @operation. If
 * @operation and then its reverse operation are applied to a buffer (in that
 * order), the buffer remains unchanged.
 *
 * @operation must be reversible for this function to be called (i.e.
 * inf_adopted_operation_is_reversible() must return TRUE).
 *
 * Return Value: The reverse operation of @operation.
 **/
InfAdoptedOperation*
inf_adopted_operation_revert(InfAdoptedOperation* operation)
{
  InfAdoptedOperationIface* iface;

  g_return_val_if_fail(INF_ADOPTED_IS_OPERATION(operation), NULL);
  g_assert(inf_adopted_operation_is_reversible(operation) == TRUE);

  iface = INF_ADOPTED_OPERATION_GET_IFACE(operation);

  /* When inf_adopted_operation_is_reversible() returns TRUE for an operation
   * it must implement revert. */
  g_assert(iface->revert != NULL);
  return (*iface->revert)(operation);
}

/* vim:set et sw=2 ts=2: */
