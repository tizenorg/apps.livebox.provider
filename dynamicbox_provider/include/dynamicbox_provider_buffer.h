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

/* Exported by dynamicbox_service library */
#include <dynamicbox_buffer.h>

#ifndef __DBOX_PROVIDER_BUFFER_H
#define __DBOX_PROVIDER_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file dynamicbox_provider_buffer.h
 * @brief This file declares API of libdynamicbox_provider library
 * @since_tizen 2.3
 */

/**
 * @addtogroup CAPI_DYNAMICBOX_PROVIDER_MODULE
 * @{
 */

/**
 * @internal
 * @brief Send request for acquiring buffer of given type.
 * @since_tizen 2.3
 * @param[in] type #DBOX_TYPE_DBOX or #DBOX_TYPE_GBAR, select the type of buffer. is it for DBOX? or GBAR?
 * @param[in] pkgname Package name of a dynamicbox instance
 * @param[in] id Instance Id
 * @param[in] auto_align
 * @param[in] handler Event handler. Viewer will send the events such as mouse down,move,up, accessibilty event, etc,. via this callback function.
 * @param[in] data Callback data for event handler
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return dynamicbox_buffer
 * @retval Dynamicbox buffer object handler
 * @retval @c NULL if it fails to acquire buffer
 * @see dynamicbox_provider_buffer_release()
 * @see dynamicbox_provider_buffer_acquire()
 */
extern dynamicbox_buffer_h dynamicbox_provider_buffer_create(dynamicbox_target_type_e type, const char *pkgname, const char *id, int auto_align, int (*handler)(dynamicbox_buffer_h, dynamicbox_buffer_event_data_t, void *), void *data);

/**
 * @internal
 * @brief Acquire a provider buffer
 * @since_tizen 2.3
 * @param[in] width Width of buffer
 * @param[in] height Height of buffer
 * @param[in] pixel_size Normally, use "4" for this. it is fixed.
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @retval #DBOX_STATUS_ERROR_NONE
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_release()
 */
extern int dynamicbox_provider_buffer_acquire(dynamicbox_buffer_h info, int width, int height, int pixel_size);

/**
 * @internal
 * @brief Acquire a extra buffer using given buffer handle
 * @since_tizen 2.3
 * @param[in] handle Dynamicbox Buffer Handle
 * @param[in] idx Index of extra buffer, start from 0
 * @param[in] width Width of buffer
 * @param[in] height Height of buffer
 * @param[in] pixel_size Pixel size in bytes
 * @return int Result status of operation
 * @retval #DBOX_STATUS_ERROR_NONE Successfully acquired
 * @retval #DBOX_STATUS_ERROR_DISABLED Extra buffer acquire/release is not supported
 * @retval #DBOX_STATUS_ERROR_FAULT Unrecoverable error occurred
 * @retval #DBOX_STATUS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMTER Invalid parameter
 * @see dynamicbox_provider_buffer_extra_release()
 * @see dynamicbox_provider_buffer_extra_resource_id()
 */
extern int dynamicbox_provider_buffer_extra_acquire(dynamicbox_buffer_h handle, int idx, int width, int height, int pixel_size);

/**
 * @internal
 * @brief Resize a buffer to given size
 * @since_tizen 2.3
 * @param[in] info Buffer handle
 * @param[in] w Width in pixles size
 * @param[in] h Height in pixels size
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameters
 * @retval #DBOX_STATUS_ERROR_NONE Successfully resized
 */
extern int dynamicbox_provider_buffer_resize(dynamicbox_buffer_h info, int w, int h);

/**
 * @internal
 * @brief Get the buffer address of given liveboDynamicboxr handle.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return address
 * @retval Address of buffer
 * @retval @c NULL
 * @see dynamicbox_provider_buffer_unref()
 */
extern void *dynamicbox_provider_buffer_ref(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Decrease the reference count of given buffer handle.
 * @since_tizen 2.3
 * @param[in] ptr Address that gets by provider_buffer_ref
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully un-referenced
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameters
 * @see dynamicbox_provider_buffer_ref()
 */
extern int dynamicbox_provider_buffer_unref(void *ptr);

/**
 * @internal
 * @brief Release the acquired buffer handle.
 * @since_tizen 2.3
 * @remarks This API will not destroy the buffer.
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameters
 * @retval #DBOX_STATUS_ERROR_NONE Successfully released
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_acquire()
 */
extern int dynamicbox_provider_buffer_release(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Release the extra buffer handle
 * @since_tizen 2.3
 * @param[in] info Buffer handle
 * @param[in] idx Index of a extra buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMTER Invalid parameters
 * @retval #DBOX_STATUS_ERROR_NONE Successfully released
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_extra_acquire()
 */
extern int dynamicbox_provider_buffer_extra_release(dynamicbox_buffer_h info, int idx);

/**
 * @internal
 * @brief Destroy Buffer Object
 * @since_tizen 2.3
 * @param[in] info Buffer handler
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully destroyed
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_release()
 */
extern int dynamicbox_provider_buffer_destroy(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Make content sync with master.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid paramter
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sync'd
 */
extern int dynamicbox_provider_buffer_sync(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Get the buffer type
 * @since_tizen 2.3
 * @param[in] info Handle of a dynamicbox buffer
 * @return target type GBAR or DBOX
 * @retval #DBOX_TYPE_DBOX Dynamicbox
 * @retval #DBOX_TYPE_GBAR Glance Bar
 * @retval #DBOX_TYPE_ERROR Undefined error
 */
extern dynamicbox_target_type_e dynamicbox_provider_buffer_type(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Get the package name of given buffer handler
 * @since_tizen 2.3
 * @param[in] info Handle of a dynamicbox buffer
 * @return const char *
 * @retval @c NULL
 * @retval pkgname package name
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_id()
 */
extern const char *dynamicbox_provider_buffer_pkgname(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Get the Instance Id of given buffer handler
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @return const char *
 * @retval instance Id in string
 * @retval @c NULL
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_pkgname()
 */
extern const char *dynamicbox_provider_buffer_id(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Give the URI of buffer information.
 * @details Every buffer information has their own URI.\n
 *          URI is consists with scheme and resource name.\n
 *          It looks like "pixmap://123456:4"\n
 *          "file:///tmp/abcd.png"\n
 *          "shm://1234"\n
 *          So you can recognize what method is used for sharing content by parse this URI.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @return const char *
 * @retval uri URI of given buffer handler
 * @retval @c NULL
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_acquire()
 */
extern const char *dynamicbox_provider_buffer_uri(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Get the size information of given buffer handler
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @param[out] w Width in pixels
 * @param[out] h Height in pixels
 * @param[out] pixel_size Bytes Per Pixels
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully get the information
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameters
 * @see dynamicbox_provider_buffer_acquire()
 * @see dynamicbox_provider_buffer_resize()
 */
extern int dynamicbox_provider_buffer_get_size(dynamicbox_buffer_h info, int *w, int *h, int *pixel_size);

/**
 * @internal
 * @brief Getting the PIXMAP id of mapped on this dynamicbox_buffer
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @return int status
 * @retval \c 0 if fails
 * @retval resource id, has to be casted to unsigned int type
 * @pre dynamicbox_provider_buffer_acquire() must be called or this function will returns @c 0
 * @see dynamicbox_provider_buffer_uri()
 * @see dynamicbox_provider_buffer_create()
 * @see dynamicbox_provider_buffer_acquire()
 */
extern unsigned int dynamicbox_provider_buffer_resource_id(dynamicbox_buffer_h info);

/**
 * @internal
 */
extern unsigned int dynamicbox_provider_buffer_extra_resource_id(dynamicbox_buffer_h info, int idx);

/**
 * @internal
 * @brief Initialize the provider buffer system
 * @since_tizen 2.3
 * @param[in] disp Display information for handling the XPixmap type.
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to initialize the provider buffer system
 * @retval #DBOX_STATUS_ERROR_NONE Successfully initialized
 * @see dynamicbox_provider_buffer_fini()
 */
extern int dynamicbox_provider_buffer_init(void *disp);

/**
 * @internal
 * @brief Finalize the provider buffer system
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully finalized
 * @see dynamicbox_provider_buffer_init()
 */
extern int dynamicbox_provider_buffer_fini(void);

/**
 * @internal
 * @brief Check whether current dynamicbox buffer support H/W(GEM) buffer
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval 1 if support
 * @retval 0 if not supported
 * @pre dynamicbox_provider_buffer_create must be called first
 * @see dynamicbox_provider_buffer_create()
 */
extern int dynamicbox_provider_buffer_is_support_hw(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Create H/W(GEM) Buffer
 * @details Allocate system buffer for sharing the contents between provider & consumer without copying overhead.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully created
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter or not supported
 * @retval #DBOX_STATUS_ERROR_FAULT Unrecoverable error occurred
 * @pre you can use this only if the provider_buffer_is_support_hw() function returns @c true(1)
 * @post N/A
 * @see dynamicbox_provider_buffer_is_support_hw()
 * @see dynamicbox_provider_buffer_destroy_hw()
 */
extern int dynamicbox_provider_buffer_create_hw(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Destroy H/W(GEM) Buffer
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully destroyed
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter
 * @see dynamicbox_provider_buffer_create_hw()
 */
extern int dynamicbox_provider_buffer_destroy_hw(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Get the H/W system mapped buffer address(GEM buffer) if a buffer support it.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return void *
 * @retval H/W system mapped buffer address
 * @retval @c NULL fails to get buffer address
 * @see dynamicbox_provider_buffer_create_hw()
 * @see dynamicbox_provider_buffer_destroy_hw()
 */
extern void *dynamicbox_provider_buffer_hw_addr(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Prepare the render buffer to write or read something on it.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_buffer_post_render()
 */
extern int dynamicbox_provider_buffer_pre_render(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Finish the render buffer acessing.
 * @since_tizen 2.3
 * @param[in] info Handle of dynamicbox buffer
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @see dynamicbox_provider_buffer_pre_render()
 */
extern int dynamicbox_provider_buffer_post_render(dynamicbox_buffer_h info);

/**
 * @internal
 * @brief Get the user data registered on buffer handler.
 * @since_tizen 2.3
 * @param[in] handle Handle of dynamicbox buffer
 * @return void *
 * @retval User data
 * @pre dynamicbox_provider_buffer_set_user_data() function must be called before
 * @see dynamicbox_provider_buffer_set_user_data()
 */
extern void *dynamicbox_provider_buffer_user_data(dynamicbox_buffer_h handle);

/**
 * @internal
 * @brief Set a user data on given buffer handler.
 * @since_tizen 2.3
 * @param[in] handle Handle of dynamicbox buffer
 * @param[in] data User data
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully updated
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter
 * @see dynamicbox_provider_buffer_user_data()
 */
extern int dynamicbox_provider_buffer_set_user_data(dynamicbox_buffer_h handle, void *data);

/**
 * @internal
 * @brief Find a buffer handler using given information
 * @since_tizen 2.3
 * @param[in] type #DBOX_TYPE_DBOX or #DBOX_TYPE_GBAR can be used
 * @param[in] pkgname Package name
 * @param[in] id Instance Id
 * @return dynamicbox_buffer
 * @retval @c NULL If it fails to find one
 * @retval handle If it found
 * @see dynamicbox_provider_buffer_create()
 */
extern dynamicbox_buffer_h dynamicbox_provider_buffer_find_buffer(dynamicbox_target_type_e type, const char *pkgname, const char *id);

/**
 * @internal
 * @brief Get the stride information of given buffer handler
 * @since_tizen 2.3
 * @param[in] info Buffer handler
 * @return int stride size in bytes
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval stride_size in bytes
 */
extern int dynamicbox_provider_buffer_stride(dynamicbox_buffer_h info);

/**
 * @brief Send the updated event to the viewer using buffer info
 * @param[in] handle Buffer handler
 * @param[in] idx Index of buffer, #DBOX_PRIMARY_BUFFER can be used to select the primary buffer if the dbox has multiple buffer
 * @param[in] region Damaged region which should be updated
 * @param[in] for_gbar 1 if it indicates Glance bar or 0 for Dynamic Box
 * @param[in] descfile Content description file if the dbox is script type one.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent update event
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_FAULT There is error and it could not be recoverable
 * @see dynamicbox_provider_buffer_create()
 */
extern int dynamicbox_provider_send_buffer_updated(dynamicbox_buffer_h handle, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile);

/**
 * @brief Send the updated event to the viewer using buffer info via given communication handler
 * @param[in] fd Communication handler, Socket file descriptor
 * @param[in] handle Buffer handler
 * @param[in] idx Index of buffer, #DBOX_PRIMARY_BUFFER can be used to select the primary buffer if the dbox has multiple buffer
 * @param[in] region Damaged region which should be updated
 * @param[in] for_gbar 1 if it indicates Glance bar or 0 for Dynamic Box
 * @param[in] descfile Content description file if the dbox is script type one.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent update event
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_FAULT There is error and it could not be recoverable
 * @see dynamicbox_provider_buffer_create()
 */
extern int dynamicbox_provider_send_direct_buffer_updated(int fd, dynamicbox_buffer_h handle, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile);

/**
 * @brief Getting the count of remained skippable frame.
 * @details If a buffer created, the viewer should not want to display unnecessary frames on the viewer,
 *          to use this, provider can gets the remained skippable frames.
 *          if the count is not reach to the ZERO, then the provider don't need to send update event to the viewer.
 *          But it highly depends on its implementations. this can be ignored by provider developer.
 * @param[in] info buffer handle
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid argument
 * @retval decimal count of skippable frames
 */
extern int dynamicbox_provider_buffer_frame_skip(dynamicbox_buffer_h info);

/**
 * @brief Clear the frame-skip value.
 * @details Sometimes viewer wants to clear this value if it detects that the updates is completed.
 * @param[in] info Buffer handle
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid argument
 * @retval #DBOX_STATUS_ERROR_NONE Successfully cleared
 */
extern int dynamicbox_provider_buffer_clear_frame_skip(dynamicbox_buffer_h info);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
/* End of a file */
