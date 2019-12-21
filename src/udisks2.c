
#include "udisks2.h"
#include "utils.h"
#include "blk_dev_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dbus/dbus.h>

typedef void (*assign_callback)(void* dst, const void* src);

struct id_off_map
{
  const char*     id;
  size_t          dbus_off;
  size_t          bdi_off;
  assign_callback assign;
};

static void
assign_string(
          void* dst
  , const void* src
)
{
        char** sdst   = dst;
  const char* const *   ssrc  = src;

  *sdst = strdup(*ssrc);
}

static void
assign_u64(
          void* dst
  , const void* src
)
{
        uint64_t* dst64 = dst;
  const uint64_t* src64 = src;

  *dst64 = *src64;
}

static void
assign_i64(
          void* dst
  , const void* src
)
{
        int64_t* dst64 = dst;
  const int64_t* src64 = src;

  *dst64 = *src64;
}

static void
assign_dbl(
          void* dst
  , const void* src
)
{
        double* ddst  = dst;
  const double* dsrc  = src;

  *ddst = *dsrc;
}

static const struct id_off_map offsets[] =
{
  {   "Model"
    , offsetof(DBusBasicValue,      str)
    , offsetof(struct blk_dev_info, model)
    , assign_string
  },
  {   "Serial"
    , offsetof(DBusBasicValue,      str)
    , offsetof(struct blk_dev_info, serial)
    , assign_string
  },
  {   "Size"
    , offsetof(DBusBasicValue,      u64)
    , offsetof(struct blk_dev_info, size_byt)
    , assign_u64
  },
  {   "SmartPowerOnSeconds"
    , offsetof(DBusBasicValue,      i64)
    , offsetof(struct blk_dev_info, smt_pwr_on_sec)
    , assign_i64
  },
  {   "SmartTemperature"
    , offsetof(DBusBasicValue,      dbl)
    , offsetof(struct blk_dev_info, smt_temp_kel)
    , assign_dbl
  },
  {
      "SmartNumBadSectors"
    , offsetof(DBusBasicValue,      i64)
    , offsetof(struct blk_dev_info, smt_bad_sect)
    , assign_i64
  }
};

static const struct id_off_map*
get_map_by_id(
    const char* id
)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(offsets); i++)
  {
    if (0 == strcmp(offsets[i].id, id))
      return &offsets[i];
  }
  return NULL;
}

static void
udisks2_blk_dev_info_fill_field(
    const struct id_off_map*    iom
  , const DBusBasicValue*       src
  ,       struct blk_dev_info*  dst
)
{
  const void* psrc  = src;
        void* pdst  = dst;

  psrc +=  iom->dbus_off;
  pdst +=  iom->bdi_off;

  iom->assign(pdst, psrc);
}

#define UDISKS_SERVICE    "org.freedesktop.UDisks2"
#define UDISKS_BASE_PATH  "/org/freedesktop/UDisks2"

DBusConnection* udisks2_dbc = NULL;

bool
udisks2_init(
)
{
  bool      rc  = false;
  DBusError err;

  assert(!udisks2_dbc);

  dbus_error_init(&err);

  udisks2_dbc = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

  if (!dbus_error_is_set(&err))
    rc = (TRUE == dbus_connection_get_is_connected(udisks2_dbc));
  else
    dbus_error_free(&err);

  return rc;
}

void
udisks2_deinit(
)
{
  assert(udisks2_dbc);
  dbus_connection_unref(udisks2_dbc);
  udisks2_dbc = NULL;
}

static struct blk_dev_info*
udisks2_get_drive_smart(
    const char* drive_dbus_path
)
{
  DBusMessage*          req;
  const char*           prop_intr[] =
                        {
                            UDISKS_SERVICE ".Drive.Ata"
                          , UDISKS_SERVICE ".Drive"
                        };
  int                   i;
  struct blk_dev_info*  bdi  = NULL;
  struct blk_dev_info*  ret  = NULL;

  if (NULL != (bdi = blk_dev_info_create()))
  {
    for (i = 0; i < ARRAY_SIZE(prop_intr); i++)
    {
      if (NULL != (req = dbus_message_new_method_call(  UDISKS_SERVICE
                                                      , drive_dbus_path
                                                      , DBUS_INTERFACE_PROPERTIES
                                                      , "GetAll"
                                                      )))
      {
        DBusError     err;
        DBusMessage*  reply;
        dbus_message_append_args( req
                                , DBUS_TYPE_STRING, &prop_intr[i]
                                , DBUS_TYPE_INVALID);

        dbus_error_init(&err);

        reply = dbus_connection_send_with_reply_and_block(  udisks2_dbc
                                                          , req
                                                          , -1
                                                          , &err
                                                         );
        if (!dbus_error_is_set(&err))
        {
          int             type  = dbus_message_get_type(reply);
          DBusMessageIter iter;
          
          if (dbus_message_iter_init(reply, &iter))
          {
            type = dbus_message_iter_get_arg_type(&iter);

            if (DBUS_TYPE_ARRAY == type)
            {
              DBusMessageIter sub;

              ret = bdi;

              dbus_message_iter_recurse(&iter, &sub);

              do
              {
                type = dbus_message_iter_get_arg_type(&sub);
                if (DBUS_TYPE_DICT_ENTRY == type)
                {
                        DBusMessageIter     sub2;
                  const struct id_off_map*  iom   = NULL;

                  dbus_message_iter_recurse(&sub, &sub2);
                  do
                  {
                    DBusBasicValue key = {};

                    type = dbus_message_iter_get_arg_type(&sub2);

                    if (dbus_type_is_basic(type))
                    {
                      dbus_message_iter_get_basic(&sub2, &key);
                      iom = get_map_by_id(key.str);
                    }
                    else if (dbus_type_is_container(type))
                    {
                      if (DBUS_TYPE_VARIANT == type)
                      {
                        DBusMessageIter sub3;

                        dbus_message_iter_recurse(&sub2, &sub3);
                        type = dbus_message_iter_get_arg_type(&sub3);

                        if (dbus_type_is_basic(type))
                        {
                          DBusBasicValue val;
                          dbus_message_iter_get_basic(&sub3, &val);
                         
                          if (iom)
                            udisks2_blk_dev_info_fill_field(iom, &val, bdi);
                        }
                      }
                    }
                  } while (dbus_message_iter_next(&sub2));
                }
              } while (dbus_message_iter_next(&sub));
            }
          }
          dbus_message_unref(reply);
        }
        else
          dbus_error_free(&err);
        dbus_message_unref(req);
      }
      else
      {
        printf("msg alloc\n");
        break;
      }
    }
  }

  if (bdi && !ret)
    blk_dev_info_destroy(bdi);

  return ret;
}

struct blk_dev_info*
udisks2_get_blk_dev_smart(
    const char* blk_dev
)
{
  char*                 udisks2_blk_dev_path;
  struct blk_dev_info*  bdi                 = NULL;

  if (-1 != asprintf(&udisks2_blk_dev_path, "%s/block_devices/%s", UDISKS_BASE_PATH, blk_dev))
  {
    DBusMessage*  req;
  
    if (NULL != (req = dbus_message_new_method_call(  UDISKS_SERVICE
                                                    , udisks2_blk_dev_path
                                                    , DBUS_INTERFACE_PROPERTIES
                                                    , "Get"
                                                    )))
    {
      const char*   prop_intr = UDISKS_SERVICE ".Block";
      const char*   prop_name = "Drive";
      DBusError     err;
      DBusMessage*  reply;

      dbus_message_append_args( req
                              , DBUS_TYPE_STRING, &prop_intr
                              , DBUS_TYPE_STRING, &prop_name
                              , DBUS_TYPE_INVALID);

      dbus_error_init(&err);

      reply = dbus_connection_send_with_reply_and_block(  udisks2_dbc
                                                        , req
                                                        , -1
                                                        , &err
                                                       );
      if (!dbus_error_is_set(&err))
      {
        int             type      = dbus_message_get_type(reply);
        DBusMessageIter iter;
        
        if (dbus_message_iter_init(reply, &iter))
        {
          type = dbus_message_iter_get_arg_type(&iter);

          if (DBUS_TYPE_VARIANT == type)
          {
            DBusMessageIter sub;

            dbus_message_iter_recurse(&iter, &sub);
            type = dbus_message_iter_get_arg_type(&sub);

            if (dbus_type_is_basic(type))
            {
              DBusBasicValue val;
              dbus_message_iter_get_basic(&sub, &val);
              bdi = udisks2_get_drive_smart(val.str);
            }
          }
        }
        dbus_message_unref(reply);
      }
      else
        dbus_error_free(&err);
      dbus_message_unref(req);
    }
    free(udisks2_blk_dev_path);
  }
  return bdi;
}

#if 0
int main(int argc, char* argv[])
{
  if (udisks2_init())
  {
    int i;
    for (i = 1; i < argc; i++)
    {
      struct blk_dev_info*  bdi;
      printf("%s:\n", argv[i]);
      if (NULL != (bdi = udisks2_get_blk_dev_smart(argv[i])))
      {
        blk_dev_info_dump(bdi);
        blk_dev_info_destroy(bdi);
      }
    }
    udisks2_deinit();
  }

  return 0;
}
#endif

