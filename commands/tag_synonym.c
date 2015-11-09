/* Copyright(C) 2015 Naoya Murakami <naoya@createfield.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301  USA
*/

#include <string.h>
#include <groonga/plugin.h>

#ifdef __GNUC__
# define GNUC_UNUSED __attribute__((__unused__))
#else
# define GNUC_UNUSED
#endif

#define SYNONYM_COLUMN_NAME "synonym"
#define SYNONYM_COLUMN_NAME_LEN 7

static grn_bool
is_table(grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
  case GRN_TABLE_NO_KEY :
    return GRN_TRUE;
  default :
    break;
  }
  return GRN_FALSE;
}

static grn_bool
is_string(grn_obj *obj)
{
  switch (obj->header.type) {
  case GRN_TYPE :
    return GRN_TRUE;
  default :
    break;
  }
  return GRN_FALSE;
}

static grn_obj *
command_tag_synonym(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
                    GNUC_UNUSED grn_user_data *user_data)
{
  GNUC_UNUSED grn_obj *flags = grn_ctx_pop(ctx);
  grn_obj *newvalue = grn_ctx_pop(ctx);
  grn_obj *oldvalue = grn_ctx_pop(ctx);
  GNUC_UNUSED grn_obj *id = grn_ctx_pop(ctx);
  grn_obj buf;
  grn_obj record;
  grn_obj *domain;
  grn_obj *table;
  grn_obj *column;
  int i,n;

  if (GRN_BULK_VSIZE(newvalue) == 0) {
    return NULL;
  }

  table = grn_ctx_at(ctx, oldvalue->header.domain);
  if (table && !is_table(table)) {
    GRN_PLUGIN_LOG(ctx, GRN_LOG_WARNING,
                   "[tag-synonym] "
                   "hooked column must be reference type");
    return NULL;
  }

  column = grn_obj_column(ctx,
                          table,
                          SYNONYM_COLUMN_NAME,
                          SYNONYM_COLUMN_NAME_LEN);
  if (!column) {
    GRN_PLUGIN_LOG(ctx, GRN_LOG_WARNING,
                   "[tag-synonym] "
                   "couldn't open synonym column");
    return NULL;
  }

  GRN_TEXT_INIT(&buf, 0);
  domain = grn_ctx_at(ctx, newvalue->header.domain);
  if (domain && is_string(domain)) {
      GRN_RECORD_INIT(&record, GRN_OBJ_VECTOR, oldvalue->header.domain);
    grn_table_tokenize(ctx, table, GRN_TEXT_VALUE(newvalue), GRN_TEXT_LEN(newvalue), &record, GRN_TRUE);
  } else if (newvalue->header.type == GRN_UVECTOR) {
    record = *newvalue;
  }

  if (is_string(domain) || newvalue->header.type == GRN_UVECTOR) {
    grn_obj value;

    GRN_RECORD_INIT(newvalue, GRN_OBJ_VECTOR, oldvalue->header.domain);
    GRN_UINT32_INIT(&value, 0);
    n = grn_vector_size(ctx, &record);
    for (i = 0; i < n; i++) {
      grn_id tid;
      tid = grn_uvector_get_element(ctx, &record, i, NULL);
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, column, tid, &value);
      if (GRN_UINT32_VALUE(&value)) {
        GRN_PLUGIN_LOG(ctx, GRN_LOG_INFO,
                       "[tag-synonym] "
                       "changed: tid %d -> %d", tid, GRN_UINT32_VALUE(&value));
        tid = GRN_UINT32_VALUE(&value);
      }
      grn_uvector_add_element(ctx, newvalue, tid, 0);
    }
    grn_obj_unlink(ctx, &value);
  } else {
    grn_id tid;
    grn_obj value;
    tid = GRN_RECORD_VALUE(newvalue);
    GRN_UINT32_INIT(&value, 0);
    grn_obj_get_value(ctx, column, tid, &value);
    if (GRN_UINT32_VALUE(&value)) {
      tid = GRN_UINT32_VALUE(&value);
      GRN_BULK_REWIND(newvalue);
      GRN_RECORD_SET(ctx, newvalue, tid);
    }
    grn_obj_unlink(ctx, &value);
  }
  grn_obj_unlink(ctx, &buf);
  grn_obj_unlink(ctx, &record);

  return NULL;
}

static grn_obj *
command_tag_synonym_delete(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
                           grn_user_data *user_data)
{
  grn_obj *var, *table, *column;
  unsigned int nhooks = 0;
  char *table_name = NULL;
  unsigned int table_len = 0;
  char *column_name = NULL;
  unsigned int column_len = 0;

  var = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    table_name = GRN_TEXT_VALUE(var);
    table_len = GRN_TEXT_LEN(var);
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "column", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    column_name = GRN_TEXT_VALUE(var);
    column_len = GRN_TEXT_LEN(var);
  }
  table = grn_ctx_get(ctx, table_name, table_len);
  column = grn_obj_column(ctx, table, column_name, column_len);

  nhooks = grn_obj_get_nhooks(ctx, column, GRN_HOOK_SET);
  if (nhooks) {
    grn_obj *hook;
    unsigned int i;
    grn_obj data;
    grn_obj buf;
    GRN_TEXT_INIT(&buf, 0);
    GRN_TEXT_INIT(&data, 0);
    for (i=0; i < nhooks; i++) {
      GRN_BULK_REWIND(&buf);
      GRN_BULK_REWIND(&data);
      hook = grn_obj_get_hook(ctx, column, GRN_HOOK_SET, i, &data);
      grn_inspect_name(ctx, &buf, hook);
      if (GRN_TEXT_LEN(&buf) == strlen("tag_synonym") &&
          strncmp(GRN_TEXT_VALUE(&buf), "tag_synonym", GRN_TEXT_LEN(&buf)) == 0) {
        grn_obj_delete_hook(ctx, column, GRN_HOOK_SET, 0);
        nhooks = grn_obj_get_nhooks(ctx, column, GRN_HOOK_SET);
        break;
      }
    }
    grn_obj_unlink(ctx, &data);
    grn_obj_unlink(ctx, &buf);
  }
  grn_ctx_output_array_open(ctx, "RESULT", 1);
  grn_ctx_output_int32(ctx, nhooks);
  grn_ctx_output_array_close(ctx);

  return NULL;
}

static grn_obj *
command_tag_synonym_add(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
                        grn_user_data *user_data)
{
  grn_obj *var, *proc, *table, *column;
  unsigned int nhooks = 0;
  char *table_name = NULL;
  unsigned int table_len = 0;
  char *column_name = NULL;
  unsigned int column_len = 0;

  var = grn_plugin_proc_get_var(ctx, user_data, "table", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    table_name = GRN_TEXT_VALUE(var);
    table_len = GRN_TEXT_LEN(var);
  }
  var = grn_plugin_proc_get_var(ctx, user_data, "column", -1);
  if (GRN_TEXT_LEN(var) != 0) {
    column_name = GRN_TEXT_VALUE(var);
    column_len = GRN_TEXT_LEN(var);
  }

  table = grn_ctx_get(ctx, table_name, table_len);
  column = grn_obj_column(ctx, table, column_name, column_len);

  {
    grn_obj *range;
    grn_obj *col;
    grn_id range_id;

    range_id = grn_obj_get_range(ctx, column);
    range = grn_ctx_at(ctx, range_id);

    if (!range) {
      GRN_PLUGIN_LOG(ctx, GRN_LOG_ERROR,
                     "[tag-synonym] "
                     "hooked column must be reference type");
      return NULL;
    }


    col = grn_obj_column(ctx,
                         range,
                         SYNONYM_COLUMN_NAME,
                         SYNONYM_COLUMN_NAME_LEN);
    if (!col) {
      GRN_PLUGIN_LOG(ctx, GRN_LOG_ERROR,
                     "[tag-synonym] "
                     "couldn't open synonym column");
      return NULL;
    }
  }

  proc = grn_ctx_get(ctx, "tag_synonym", -1);
  grn_obj_add_hook(ctx, column, GRN_HOOK_SET, 0, proc, 0);

  grn_ctx_output_array_open(ctx, "RESULT", 1);
  nhooks = grn_obj_get_nhooks(ctx, column, GRN_HOOK_SET);
  grn_ctx_output_int32(ctx, nhooks);
  grn_ctx_output_array_close(ctx);

  return NULL;
}

grn_rc
GRN_PLUGIN_INIT(GNUC_UNUSED grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_expr_var vars[2];

  grn_plugin_command_create(ctx, "tag_synonym", -1, command_tag_synonym, 0, vars);

  grn_plugin_expr_var_init(ctx, &vars[0], "table", -1);
  grn_plugin_expr_var_init(ctx, &vars[1], "column", -1);
  grn_plugin_command_create(ctx, "tag_synonym_add", -1, command_tag_synonym_add, 2, vars);

  grn_plugin_expr_var_init(ctx, &vars[0], "table", -1);
  grn_plugin_expr_var_init(ctx, &vars[1], "column", -1);
  grn_plugin_command_create(ctx, "tag_synonym_delete", -1, command_tag_synonym_delete, 2, vars);

  return ctx->rc;
}

grn_rc
GRN_PLUGIN_FIN(GNUC_UNUSED grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
