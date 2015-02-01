/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dynamicbox_service.h>

#ifndef __DBOX_PROVIDER_APP_H
#define __DBOX_PROVIDER_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file dynamicbox_provider_app.h
 * @brief This file declares API of libdynamicbox_provider_app library
 * @since_tizen 2.3
 */

/**
 * @addtogroup DYNAMICBOX_PROVIDER_APP_MODULE
 * @{
 */

/**
 * @brief Dynamicbox Life-cycle event table
 * @since_tizen 2.3
 */
typedef struct dynamicbox_provider_event_callback { /**< Event callback table for Dynamicbox Provider App */
    struct _dynamicbox { /**< Event callbacks for Dynamic Box */
        int (*create)(const char *id, const char *content, int w, int h, void *data); /**< Creating a new Dynamic Box */
        int (*resize)(const char *id, int w, int h, void *data); /**< Resizing the Dynamic Box */
        int (*destroy)(const char *id, dynamicbox_destroy_type_e reason, void *data); /**< Destroying the Dynamic Box */
    } dbox;    /**< 'dbox' is short from DynamicBox */

    struct _glancebar { /**< Event callbacks for Glance Bar */
        int (*create)(const char *id, int w, int h, double x, double y, void *data); /**< Create a Glance Bar */
        int (*resize_move)(const char *id, int w, int h, double x, double y, void *data); /**< Resize & Move the Glance Bar */
        int (*destroy)(const char *id, int reason, void *data); /**< Destroy the Glance Bar */
    } gbar;    /**< Short from Glance Bar */

    int (*clicked)(const char *id, const char *event, double x, double y, double timestamp, void *data); /**< If viewer calls dynamicbox_click function, this event callback will be called */

    int (*update)(const char *id, const char *content, int force, void *data); /**< Updates a Dynamic Box */
    int (*script_event)(const char *id, const char *emission, const char *source, struct dynamicbox_event_info *info, void *data); /**< User events Dynamic Box */

    int (*pause)(const char *id, void *data); /**< Pause */
    int (*resume)(const char *id, void *data); /**< Resume */

    void (*connected)(void *data); /**< When a provider is connected to master service, this callback will be called */
    void (*disconnected)(void *data); /**< When a provider is disconnected from the master service, this callback will be called */

    void *data; /**< Callback data */
} *dynamicbox_provider_event_callback_t;

/**
 * @brief Initialize the provider app.
 * @since_tizen 2.3
 * @param[in] service Service handle which is comes from app_service event callback
 * @param[in[ table Event callback table
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int type
 * @retval #DBOX_STATUS_ERROR_NONE Succeed to initialize the provider app.
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to initialize the provider app.
 * @retval #DBOX_STATUS_ERROR_PERMISSION Lack of access privileges
 */
extern int dynamicbox_provider_app_init(app_control_h service, dynamicbox_provider_event_callback_t table);

/**
 * @brief Check the initialization status of dynamicbox provider app.
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval 1 if it is initialized
 * @retval 0 if it is not initialized
 */
extern int dynamicbox_provider_app_is_initialized(void);

/**
 * @brief Finiailze the provider app
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privielge/dynamicbox.provider
 * @return void
 */
extern void dynamicbox_provider_app_fini(void);

/**
 * @brief Set the control mode of master provider for provider apps.
 * @since_tizen 2.3
 * @param[in] flag 1 if the app's lifecycle will be managed by itself, or 0, the master will manage the life-cycle of a provider app
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int type
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid parameter
 * @retval #DBOX_STATUS_ERROR_FAULT Unrecoverable error
 * @retval #DBOX_STATUS_ERROR_NONE Successfully done
 */
extern int dynamicbox_provider_app_manual_control(int flag);

/**
 * @brief Send the "updated" event for Dynamic Box manually
 * @since_tizen 2.3
 * @param[in] id Instance Id
 * @param[in] idx Index of updated buffer
 * @param[in] w Width
 * @param[in] h Height
 * @param[in] for_gbar 1 for sending updated event of GBAR or 0 for DBox
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int type
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid argument
 * @retval #DBOX_STATUS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #DBOX_STATUS_ERROR_FAULT Unrecoverable error
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @see dynamicbox_provider_app_gbar_updated()
 */
extern int dynamicbox_provider_app_updated(const char *id, int idx, int x, int y, int w, int h, int for_gbar);

/**
 * @brief Send the "updated" event for Dynamic Box manually
 * @since_tizen 2.3
 * @param[in] handle Buffer handler
 * @param[in] idx Index of updated buffer
 * @param[in] w Width
 * @param[in] h Height
 * @param[in] for_gbar 1 for sending updated event of GBAR or 0 for DBox
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int type
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid argument
 * @retval #DBOX_STATUS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #DBOX_STATUS_ERROR_FAULT Unrecoverable error
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @see dynamicbox_provider_app_gbar_updated()
 */
extern int dynamicbox_provider_app_buffer_updated(dynamicbox_buffer_h handle, int idx, int x, int y, int w, int h, int for_gbar);

/**
 * @brief Send the extra info to the Dynamic Box manually\n
 * @since_tizen 2.3
 * @param[in] priority Priority of contens (0.0 <= p <= 1.0)
 * @param[in] content_info Content info, this content info will be managed by viewer.
 * @param[in] title When an accessibility mode is turned on, this string will be read.
 * @param[in] icon Alternative Icon filename, if a viewer doesn't support dynamicbox, it can display simple icon instead of a box. this will be used for it.
 * @param[in] name Alternative App name, if a viewer doesn't support dynamicbox, it can display short name, this will be used for it.
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int type
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid argument
 * @retval #DBOX_STATUS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #DBOX_STATUS_ERROR_FAULT Unrecoverable error
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @see dynamicbox_provider_app_dbox_updated()
 */
extern int dynamicbox_provider_app_extra_info_updated(const char *id, double priority, const char *content_info, const char *title, const char *icon, const char *name);

/**
 * @brief Set private data to given Instance.
 * @since_tizen 2.3
 * @param[in] id Instance Id
 * @param[in] data Private Data
 * @return int type
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid argument
 * @retval #DBOX_STATUS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #DBOX_STATUS_ERROR_NOT_EXIST Instance is not exists
 * @retval #DBOX_STATUS_ERROR_NONE Successfully registered
 * @see dynamicbox_provider_app_data()
 */
extern int dynamicbox_provider_app_set_data(const char *id, void *data);

/**
 * @brief Get the private data from give Instance.
 * @since_tizen 2.3
 * @param[in] id Instance Id
 * @return void* Address
 * @retval Address Private data
 * @retval @c NULL fails to get private data or not registered
 * @see dynamicbox_dynamicbox_provider_app_set_data()
 */
extern void *dynamicbox_provider_app_data(const char *id);

/**
 * @brief Get the list of private data
 * @details This function will returns list of data, list type is Eina_List.
 * @since_tizen 2.3
 * @return void* Eina_List *
 * @retval Address Eina_List *
 * @retval @c NULL if there is no list of data
 * @post Returned list must has to be freed by function eina_list_free or macro EINA_LIST_FREE
 * @see dynamicbox_provider_app_set_data()
 */
extern void *dynamicbox_provider_app_data_list(void);

/**
 * @internal
 * @brief Creating a buffer handler for given livebox instance.
 * @since_tizen 2.3
 * @param[in] id Dynamic Box Instnace Id
 * @param[in] for_pd 1 for Glance Bar or 0
 * @param[in] handle Event Handler. All events will be delievered via this event callback.
 * @param[in] data Callback Data
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return Buffer Object
 * @retval @c NULL if it fails to create a buffer
 * @retval addr Address of dynamic box buffer object
 * @see dynamicbox_provider_app_acquire_buffer()
 * @see dynamicbox_provider_app_release_buffer()
 * @see dynamicbox_provider_app_destroy_buffer()
 */
extern dynamicbox_buffer_h dynamicbox_provider_app_create_buffer(const char *id, int for_pd, int (*handle)(dynamicbox_buffer_h, dynamicbox_buffer_event_data_t, void *), void *data);

/**
 * @internal
 * @brief Acquire buffer
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @param[in] w Buffer Width
 * @param[in] h Buffer Height
 * @param[in] bpp Bytes Per Pixels
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid parameters are used
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to acquire buffer. Unable to recover from error
 * @retval #DBOX_STATUS_ERROR_NONE If it creates buffer successfully
 * @pre dynamicbox_provider_app_create_buffer must be called first to create a handle
 */
extern int dynamicbox_provider_app_acquire_buffer(dynamicbox_buffer_h handle, int w, int h, int bpp);

/**
 * @internal
 * @brief Acquire extra buffer
 * @details if you need more buffer to rendering scenes, you can allocate new one using this.\n
 *          But the maximum count of allocatable buffer is limited. it depends on system setting.\n
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer handle
 * @param[in] idx Index of a buffer, 0 <= idx <= SYSTEM_DEFINE
 * @param[in] w Buffer width
 * @param[in] h Buffer height
 * @param[in] bpp Bytes Per Pixel
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid parameters are used
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to acquire buffer. Unable to recover from error
 * @retval #DBOX_STATUS_ERROR_NONE If it creates buffer successfully
 * @pre dynamicbox_provider_app_create_buffer must be called first to create a handle
 */
extern int dynamicbox_provider_app_acquire_extra_buffer(dynamicbox_buffer_h handle, int idx, int w, int h, int bpp);

/**
 * @internal
 * @brief Release buffer
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMTER Invalid parameters are used
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to release buffer, Unable to recover from error
 * @retval #DBOX_STATUS_ERROR_NOT_EXIST Given buffer handle is not exists
 * @retval #DBOX_STATUS_ERROR_NONE If it releases buffer successfully
 * @see dynamicbox_provider_app_create_buffer()
 * @see dynamicbox_provider_app_acquire_buffer()
 * @see dynamicbox_provider_app_destroy_buffer()
 */
extern int dynamicbox_provider_app_release_buffer(dynamicbox_buffer_h handle);

/**
 * @internal
 * @brief Release buffer
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @param[in] idx Index of a buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMTER Invalid parameters are used
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to release buffer, Unable to recover from error
 * @retval #DBOX_STATUS_ERROR_NOT_EXIST Given buffer handle is not exists
 * @retval #DBOX_STATUS_ERROR_NONE If it releases buffer successfully
 * @see dynamicbox_provider_app_create_buffer()
 * @see dynamicbox_provider_app_acquire_buffer()
 * @see dynamicbox_provider_app_destroy_buffer()
 */
extern int dynamicbox_provider_app_release_extra_buffer(dynamicbox_buffer_h handle, int idx);

/**
 * @internal
 * @brief Destroy buffer
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMTER Invalid parameter
 * @retval #DBOX_STATUS_ERROR_NONE Successfully destroyed
 * @see dynamicbox_provider_app_create_buffer()
 * @see dynamicbox_provider_app_acquire_buffer()
 * @see dynamicbox_provider_app_destroy_buffer()
 * @pre dynamicbox_provider_app_create_buffer() must be successfully called and before call this, the buffer should be released first.
 */
extern int dynamicbox_provider_app_destroy_buffer(dynamicbox_buffer_h handle);

/**
 * @internal
 * @brief Get the resource Id
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @return unsigned int Resource Id
 * @retval 0 if it fails to get resource Id
 * @retval positive if it succeed to get the resource Id
 * @see dynamicbox_provider_app_create_buffer()
 * @see dynamicbox_provider_app_acquire_buffer()
 * @pre dynamicbox_provider_app_acquire_buffer() must be successfully called.
 */
extern unsigned int dynamicbox_provider_app_buffer_resource_id(dynamicbox_buffer_h handle);

/**
 * @internal
 * @brief Get the resource Id for extra buffer
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @return unsigned int Resource Id
 * @retval 0 if it fails to get resource Id
 * @retval positive if it succeed to get the resource Id
 * @see dynamicbox_provider_app_create_buffer()
 * @see dynamicbox_provider_app_acquire_buffer()
 * @pre dynamicbox_provider_app_acquire_buffer() must be successfully called.
 */
extern unsigned int dynamicbox_provider_app_extra_buffer_resource_id(dynamicbox_buffer_h handle, int idx);

/**
 * @internal
 * @brief Send a result of accessibility event processing
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @param[in] status enum dynamicbox_access_status (dynamicbox_service.h)
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid parameter
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send access result status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @see dynamicbox_provider_app_send_key_status()
 */
extern int dynamicbox_provider_app_send_access_status(dynamicbox_buffer_h handle, int status);

/**
 * @internal
 * @brief Send a result of key event processing
 * @since_tizen 2.3
 * @param[in] handle Dynamic Box Buffer Handle
 * @param[in] status enum dynamicbox_key_status (dynamicbox_service.h)
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid parameter
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send key processing result status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @see dynamicbox_provider_app_send_access_status()
 */
extern int dynamicbox_provider_app_send_key_status(dynamicbox_buffer_h handle, int status);

/**
 * @brief Send the request to freeze/thaw scroller to the homescreen
 * @details If a viewer doesn't implement service routine for this request, this doesn't make any changes.
 * @param[in] id Instance Id
 * @param[in] seize 1 for freeze, 0 for thaw
 * @privlevel public
 * @privilege %http://tizen.org/privilege/core/dynamicbox.provider
 * @return int type
 * @return #DBOX_STATUS_ERROR_NONE Successfully sent
 */
extern int dynamicbox_provider_app_hold_scroll(const char *id, int seize);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

/* End of a file */
