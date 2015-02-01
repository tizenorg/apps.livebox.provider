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

#ifndef __PROVIDER_H
#define __PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup CAPI_DYNAMICBOX_PROVIDER_MODULE
 * @{
 */

/**
 * @brief
 * Update the provider control
 * @see provider_control()
 * THIS ENUM MUST HAS TO BE SYNC'd WITH THE DATA-PROVIDER-MASTER
 */
enum PROVIDER_CTRL {
    PROVIDER_CTRL_DEFAULT = 0x00,			/**< Set default control operation */
    PROVIDER_CTRL_MANUAL_TERMINATION = 0x01,	/**< Terminate process manually */
    PROVIDER_CTRL_MANUAL_REACTIVATION = 0x02,	/**< Reactivate process manually */
};

#if !defined(__SCRIPT_EVENT_INFO)
#define __SCRIPT_EVENT_INFO
/**
 * @brief
 * Text signal & Content event uses this data structure.
 */
struct event_info {
    struct pointer {
	double x; /**< X value of current mouse(touch) position */
	double y; /**< Y value of current mouse(touch) position */
	int down; /**< Is it pressed(1) or not(0) */
    } pointer;

    struct part {
	double sx; /**< Pressed object's left top X */
	double sy; /**< Pressed object's left top Y */
	double ex; /**< Pressed object's right bottom X */
	double ey; /**< Pressed object's right bottom Y */
    } part;
};
#endif

struct event_arg {
    enum event_type {
	EVENT_NEW, /**< Master will send this to create a new livebox instance */
	EVENT_RENEW, /**< If the master detects any problems of your slave, it will terminate slave provider.
		       and after reactivating the provider slave, this request will be delievered to create
		       a livebox instance again */

	EVENT_DELETE, /**< Master will send this to delete a livebox instance */

	EVENT_CONTENT_EVENT, /**< Any events are generated from your livebox or PD, this event will be sent to you */
	EVENT_CLICKED, /**< If a livebox is clicked, the master will send this event */
	EVENT_TEXT_SIGNAL, /**< Text type livebox or PD will generate this event */

	EVENT_RESIZE, /**< If a livebox is resized, the master will send this event */
	EVENT_SET_PERIOD, /**< To change the update period of a livebox */
	EVENT_CHANGE_GROUP, /**< To change the group(cluster/sub-cluster) of a livebox */
	EVENT_PINUP, /**< To make pin up of a livebox */

	EVENT_UPDATE_CONTENT, /**< It's time to update the content of a livebox */

	EVENT_PAUSE, /**< Freeze all timer and go to sleep mode */
	EVENT_RESUME, /**< Thaw all timer and wake up */

	EVENT_PD_CREATE, /**< Only for the buffer type */
	EVENT_PD_DESTROY, /**< Only for the buffer type */
	EVENT_PD_MOVE, /**< Only for the buffer type */

	EVENT_LB_PAUSE, /**< Freeze the update timer of a specified livebox */
	EVENT_LB_RESUME, /**< Thaw the update timer of a specified livebox */
    } type;
    const char *pkgname; /**< Package name of a livebox */
    const char *id; /**< Instance Id of a livebox */

    union event_data {
	struct pd_create {
	    int w; /**< PD buffer is created with width "w" */
	    int h; /**< PD buffer is created with height "h" */

	    double x; /**< Relative X position of a livebox from this PD */
	    double y; /**< Relative Y position of a livebox from this PD */
	} pd_create;

	struct pd_destroy {
	    int reason;
	} pd_destroy;

	struct pd_move {
	    int w; /**< PD buffer width */
	    int h; /**< PD buffer height */

	    double x; /**< Relative X position of a livebox from this PD */
	    double y; /**< Relative Y position of a livebox from this PD */
	} pd_move;

	struct lb_create {
	    const char *content; /**< Content info */
	    int timeout; /**< Timeout */
	    int has_script; /**< Livebox has script (buffer would be created from the master) */
	    double period; /**< Update period */
	    const char *cluster; /**< Cluster ID of this livebox instance */
	    const char *category; /**< Sub-cluster ID of this livebox instance */
	    int skip_need_to_create; /**< Is this livebox need to check the "need_to_create"? */
	    int width; /**< Width of a livebox content in pixels */
	    int height; /**< Height of a livebox content in pixels */
	    const char *abi; /**< ABI tag of this livebox */
	    char *out_content; /**< Output content */
	    char *out_title; /**< Output title */
	    int out_is_pinned_up; /**< Is this pinned up? */
	    char *direct_addr;
	} lb_create; /**< "new" */

	struct lb_recreate {
	    const char *content; /**< Content info */
	    int timeout; /**< Timeout */
	    int has_script; /**< Livebox has script (buffer would be created from the master) */
	    double period; /**< Update period */
	    const char *cluster; /**< Cluster ID of this livebox instance */
	    const char *category; /**< Sub-cluster ID of this livebox instance */
	    int width; /**< Width of a livebox content in pixels */
	    int height; /**< Height of a livebox content in pixels */
	    const char *abi; /**< ABI tag of this livebox */
	    char *out_content; /**< Output content */
	    char *out_title; /**< Output title */
	    int out_is_pinned_up; /**< Is this pinned up? */
	    int hold_scroll; /**< The scroller which is in viewer is locked */
	    int active_update; /**< Need to know, current update mode */
	    char *direct_addr;
	} lb_recreate; /**< renew */

	struct lb_destroy { /**< This enumeration value must has to be sync with data-provider-master */
	    enum instance_destroy_type {
		INSTANCE_DESTROY_DEFAULT = 0x00, /**< Deleted */
		INSTANCE_DESTROY_PKGMGR = 0x01,	/**< For the compatibility */
		INSTANCE_DESTROY_UPGRADE = 0x01, /**< Deleted for upgrading */
		INSTANCE_DESTROY_UNINSTALL = 0x02, /**< Deleted by uninstalling */
		INSTANCE_DESTROY_TERMINATE = 0x03, /**< Deleted for reboot device */
		INSTANCE_DESTROY_FAULT = 0x04, /**< Deleted by system-fault */
		INSTANCE_DESTROY_TEMPORARY = 0x05, /**< Temporarly deleted, will be created again */
		INSTANCE_DESTROY_UNKNOWN = 0x06, /**< Undefined reason */
	    } type; /**< Delete type */
	} lb_destroy; /**< Destroyed */

	struct content_event {
	    const char *emission; /**< Event string */
	    const char *source; /**< Object ID which makes event */
	    struct event_info info;
	} content_event; /**< script */

	struct clicked {
	    const char *event; /**< Event type, currently only "click" supported */
	    double timestamp; /**< Timestamp of event occurred */
	    double x; /**< X position of the click event */
	    double y; /**< Y position of the click event */
	} clicked; /**< clicked */

	struct text_signal {
	    const char *emission; /**< Event string */
	    const char *source; /**< Object ID which makes event */
	    struct event_info info;
	} text_signal; /**< text_signal */

	struct resize {
	    int w; /**< New width of a livebox */
	    int h; /**< New height of a livebox */
	} resize; /**< resize */

	struct set_period {
	    double period; /**< New period */
	} set_period; /**< set_period */

	struct change_group {
	    const char *cluster; /**< Cluster information is changed */
	    const char *category; /**< Sub-cluster information is changed */
	} change_group; /**< change_group */

	struct pinup {
	    int state; /**< Current state of Pin-up */
	    char *content_info; /**< out value */
	} pinup; /**< pinup */

	struct update_content {
	    const char *cluster; /**< Cluster information */
	    const char *category; /**< Sub-cluster information */
	    const char *content; /**< Content information */
	    int force; /**< Updated by fault */
	} update_content; /**< update_content */

	struct pause {
	    double timestamp; /**< Timestamp of the provider pause event occurred */
	} pause; /**< pause */

	struct resume {
	    double timestamp; /**< Timestamp of the provider resume event occurred */
	} resume; /**< resume */

	struct lb_pause {
	    /**< Livebox is paused */
	} lb_pause;

	struct lb_resume {
	    /**< Livebox is resumed */
	} lb_resume;

	struct update_mode {
	    int active_update; /**< 1 for Active update mode or 0 */
	} update_mode; /**< Update mode */
    } info;
};

struct event_handler {
    int (*lb_create)(struct event_arg *arg, int *width, int *height, double *priority, void *data); /**< new */
    int (*lb_destroy)(struct event_arg *arg, void *data); /**< delete */

    /* Recover from the fault of slave */
    int (*lb_recreate)(struct event_arg *arg, void *data); /**< Re-create */

    int (*lb_pause)(struct event_arg *arg, void *data); /**< Pause Livebox */
    int (*lb_resume)(struct event_arg *arg, void *data); /**< Resume Livebox */

    int (*content_event)(struct event_arg *arg, void *data); /**< Content event */
    int (*clicked)(struct event_arg *arg, void *data); /**< Clicked */
    int (*text_signal)(struct event_arg *arg, void *data); /**< Text Signal */
    int (*resize)(struct event_arg *arg, void *data); /**< Resize */
    int (*set_period)(struct event_arg *arg, void *data); /**< Set Period */
    int (*change_group)(struct event_arg *arg, void *data); /**< Change group */
    int (*pinup)(struct event_arg *arg, void *data); /**< Pin up */
    int (*update_content)(struct event_arg *arg, void *data); /**< Update content */

    int (*pause)(struct event_arg *arg, void *data); /**< Pause service provider */
    int (*resume)(struct event_arg *arg, void *data); /**< Resume service provider */

    int (*disconnected)(struct event_arg *arg, void *data); /**< Disconnected from master */
    int (*connected)(struct event_arg *arg, void *data); /**< Connected to master */

    /**
     * @note
     * Only for the buffer type
     */
    int (*pd_create)(struct event_arg *arg, void *data); /**< PD is created */
    int (*pd_destroy)(struct event_arg *arg, void *data); /**< PD is destroyed */
    int (*pd_move)(struct event_arg *arg, void *data); /**< PD is moved */

    int (*update_mode)(struct event_arg *arg, void *data); /**< Update mode */
};

/**
 * @brief Initialize the provider service
 * @param[in] disp XDisplay object, if you don't know what this is, set @c NULL
 * @param[in] name Slave name which is given by the master provider.
 * @param[in] table Event handler table
 * @param[in] data callback data
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully initialized
 * @retval #LB_STATUS_ERROR_INVALID Invalid paramters
 * @retval #LB_STATUS_ERROR_MEMORY Out of memory
 * @retval #LB_STATUS_ERROR_ALREADY Already initialized
 * @retval #LB_STATUS_ERROR_FAULT Failed to initialize
 * @see provider_fini()
 * @see provider_init_with_options()
 */
extern int provider_init(void *disp, const char *name, struct event_handler *table, void *data);

/**
 * @brief Initialize the provider service
 * @details Some provider doesn't want to access environment value.\n
 *          This API will get some configuration info via argument instead of environment value.
 * @param[in] disp XDisplay object, if you don't know what this is, set @c NULL
 * @param[in] name Slave name which is given by the master provider.
 * @param[in] table Event handler table
 * @param[in] data callback data
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully initialized
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_MEMORY Out of memory
 * @retval #LB_STATUS_ERROR_ALREADY Already initialized
 * @retval #LB_STATUS_ERROR_FAULT Failed to initialize
 * @see provider_fini()
 * @see provider_init()
 */
extern int provider_init_with_options(void *disp, const char *name, struct event_handler *table, void *data, int prevent_overwrite, int com_core_use_thread);

/**
 * @brief Finalize the provider service
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return void *
 * @retval callback data which is registered via provider_init
 * @see provider_init()
 */
extern void *provider_fini(void);

/**
 * @brief Send the hello signal to the master
 * @remarks\n
 *        Master will activate connection of this only if you send this hello event.\n
 *        or the master will reject all requests from your provider.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_FAULT Failed to initialize
 */
extern int provider_send_hello(void);

/**
 * @brief Send the ping message to the master to notify that your provider is working properly.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID provider is not initialized
 * @retval #LB_STATUS_ERROR_FAULT Failed to send ping message
 */
extern int provider_send_ping(void);

/**
 * @brief Send the updated event to the master about livebox.
 * @details\n
 *  If you call this function, the output filename which is pointed by @a id will be renamed to uniq filename.\n
 *  So you cannot access the output file after this function calls.\n
 *  But if the @a prevent_overwrite option is enabled, the output filename will not be changed.\n
 *  This is only happens when the livebox uses file for content sharing method as image or description.
 * @param[in] pkgname Package name which is an updated livebox.
 * @param[in] id Instance ID of an updated livebox
 * @param[in] w Width of an updated content
 * @param[in] h Height of an updated content
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent update event
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_FAULT There is error and it could not be recoverable
 * @see provider_send_desc_updated()
 */
extern int provider_send_updated(const char *pkgname, const char *id, int w, int h);

/**
 * @brief Send the updated event to the client directly.
 * @details\n
 *  If you call this function, the output filename which is pointed by @a id will be renamed to uniq filename.\n
 *  So you cannot access the output file after this function calls.\n
 *  But if the @a prevent_overwrite option is enabled, the output filename will not be changed.\n
 *  This is only happens when the livebox uses file for content sharing method as image or description.
 * @param[in] fd connection handler
 * @param[in] pkgname Package name which is an updated livebox
 * @param[in] id Instance ID of an updated livebox
 * @param[in] w Width of an updated content
 * @param[in] h Height of an updated content
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent update event
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_FAULT There is error and it could not be recoverable
 * @see provider_send_desc_updated()
 */
extern int provider_send_direct_updated(int fd, const char *pkgname, const char *id, int w, int h);

/**
 * @brief Send extra information to the master about livebox
 * @param[in] priority Priority of an updated content
 * @param[in] content_info Content string
 * @param[in] title Title string that is summarizing the content to a short text
 * @param[in] icon Absolute path of an icon file to use it for representing the contents of a box alternatively
 * @param[in] name Name for representing the contents of a box alternatively
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent update event
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_FAULT There is error and it could not be recoverable
 */
extern int provider_send_extra_info(const char *pkgname, const char *id, double priority, const char *content_info, const char *title, const char *icon, const char *name);

/**
 * @brief Send the description data updated event to the master about PD.
 * @details If you call this function, the descfile will be renamed.
 * @param[in] pkgname Package name of an updated livebox.
 * @param[in] id Instance ID of an updated livebox.
 * @param[in] descfile The filename of a description file.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully updated
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_MEMORY Out of memory
 * @retval #LB_STATUS_ERROR_FAULT Failed to send updated event (PD)
 * @see provider_send_updated()
 */
extern int provider_send_desc_updated(const char *pkgname, const char *id, const char *descfile);

/**
 * @brief Send the description data updated event to the client about PD directly.
 * @details If you call this function, the descfile will be renamed.
 * @param[in] fd connection handler.
 * @param[in] pkgname Package name of an updated livebox.
 * @param[in] id Instance ID of an updated livebox.
 * @param[in] descfile The filename of a description file.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully updated
 * @retval #LB_STATUS_ERROR_INVALID Invalid arguments
 * @retval #LB_STATUS_ERROR_MEMORY Out of memory
 * @retval #LB_STATUS_ERROR_FAULT Failed to send updated event (PD)
 * @see provider_send_updated()
 */
extern int provider_send_direct_desc_updated(int fd, const char *pkgname, const char *id, const char *descfile);

/**
 * @brief If the client requests async update mode, service provider must has to send update begin when it really start to update its contents
 * @param[in] pkgname Package name of livebox
 * @param[in] id Instance Id of livebox
 * @param[in] priority Priority of current content of livebox
 * @param[in] content_info Content info could be come again via livebox_create() interface
 * @param[in] title A sentence for describing content of livebox
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_send_lb_update_end()
 */
extern int provider_send_lb_update_begin(const char *pkgname, const char *id, double priority, const char *content_info, const char *title);

/**
 * @brief If the client requests async update mode, service provider must has to send update end when the update is done
 * @param[in] pkgname Package name of livebox
 * @param[in] id Instance Id of livebox
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_send_lb_update_begin()
 */
extern int provider_send_lb_update_end(const char *pkgname, const char *id);

/**
 * @brief If the client requests async update mode, service provider must has to send update begin when it really start to update its contents
 * @param[in] pkgname Package name of livebox
 * @param[in] id Instance Id of livebox
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_send_pd_update_end()
 */
extern int provider_send_pd_update_begin(const char *pkgname, const char *id);

/**
 * @brief If the client requests async update mode, service provider must has to send update end when it really finish the update its contents
 * @param[in] pkgname Package name of livebox
 * @param[in] id Instance Id of livebox
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_send_pd_update_begin()
 */
extern int provider_send_pd_update_end(const char *pkgname, const char *id);

/**
 * @brief Send the deleted event of specified livebox instance
 * @param[in] pkgname Package name of the livebox
 * @param[in] id Livebox instance ID
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_send_faulted()
 * @see provider_send_hold_scroll()
 */
extern int provider_send_deleted(const char *pkgname, const char *id);

/**
 * @brief If you want to use the fault management service of the master provider,\n
 *        Before call any functions of a livebox, call this.
 * @param[in] pkgname Package name of the livebox
 * @param[in] id Instance ID of the livebox
 * @param[in] funcname Function name which will be called
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_send_call()
 */
extern int provider_send_ret(const char *pkgname, const char *id, const char *funcname);

/**
 * @brief If you want to use the fault management service of the master provider,\n
 *        After call any functions of a livebox, call this.
 * @param[in] pkgname Package name of the livebox
 * @param[in] id Instance ID of the livebox
 * @param[in] funcname Function name which is called by the slave
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID Invalid paramters
 * @retval #LB_STATUS_ERROR_FAULT Failed to send a request
 * @see provider_send_ret()
 */
extern int provider_send_call(const char *pkgname, const char *id, const char *funcname);

/**
 * @brief If you want to send the fault event to the master provider,
 *        Use this API
 * @param[in] pkgname Package name of the livebox
 * @param[in] id ID of the livebox instance
 * @param[in] funcname Reason of the fault error
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameters
 * @retval #LB_STATUS_ERROR_FAULT Failed to send a request
 * @see provider_send_deleted()
 * @see provider_send_hold_scroll()
 * @see provider_send_access_status()
 * @see provider_send_key_status()
 * @see provider_send_request_close_pd()
 */
extern int provider_send_faulted(const char *pkgname, const char *id, const char *funcname);

/**
 * @brief If you want notify to viewer to seize the screen,
 *        prevent moving a box from user event
 * @param[in] pkgname Package name of the livebox
 * @param[in] id ID of the livebox instance
 * @param[in] seize 1 if viewer needs to hold a box, or 0
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameters
 * @retval #LB_STATUS_ERROR_FAULT Failed to send a request
 * @see provider_send_faulted()
 * @see provider_send_deleted()
 * @see provider_send_access_status()
 * @see provider_send_key_status()
 * @see provider_send_request_close_pd()
 */
extern int provider_send_hold_scroll(const char *pkgname, const char *id, int seize);

/**
 * @brief Notify to viewer for accessiblity event processing status.
 * @param[in] pkgname Package name of livebox
 * @param[in] id Instance Id of livebox
 * @param[in] status \n
 *	You should returns one in below list, those are defined in livebox-service.h header\n
 *	#LB_ACCESS_STATUS_ERROR If there is some error while processing accessibility events\n
 *	#LB_ACCESS_STATUS_DONE Successfully processed\n
 *	#LB_ACCESS_STATUS_FIRST Reach to the first focusable object\n
 *	#LB_ACCESS_STATUS_LAST Reach to the last focusable object\n
 *	#LB_ACCESS_STATUS_READ Read done
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter
 * @retval #LB_STATUS_ERROR_FAULT Failed to send a status
 * @see provider_send_deleted()
 * @see provider_send_faulted()
 * @see provider_send_hold_scroll()
 * @see provider_send_key_status()
 * @see provider_send_request_close_pd()
 */
extern int provider_send_access_status(const char *pkgname, const char *id, int status);

/**
 * @brief Notify to viewer for key event processing status.
 * @param[in] pkgname Package name of livebox
 * @param[in] id Instance Id of livebox
 * @param[in] status \n
 *	Send the key event processing result to the viewer\n
 *	#LB_KEY_STATUS_ERROR If there is some error while processing key event\n
 *	#LB_KEY_STATUS_DONE Key event is successfully processed\n
 *	#LB_KEY_STATUS_FIRST Reach to the first focusable object\n
 *	#LB_KEY_STATUS_LAST Reach to the last focusable object
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_INVALID Invalid paramter
 * @retval #LB_STATUS_ERROR_FAULT Failed to send a status
 * @see provider_send_deleted()
 * @see provider_send_faulted()
 * @see provider_send_hold_scroll()
 * @see provider_send_access_status()
 * @see provider_send_request_close_pd()
 */
extern int provider_send_key_status(const char *pkgname, const char *id, int status);

/**
 * @brief Send request to close the PD if it is opened.
 * @param[in] pkgname Package name to send a request to close the PD
 * @param[in] id Instance Id of livebox which should close its PD
 * @param[in] reason Reserved for future use
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully sent
 * @retval #LB_STATUS_ERROR_FAULT Failed to send request to the viewer
 * @retval #LB_STATUS_ERROR_INVALID Invalid argument
 * @see provider_send_deleted()
 * @see provider_send_faulted()
 * @see provider_send_hold_scroll()
 * @see provider_send_access_status()
 * @see provider_send_key_status()
 */
extern int provider_send_request_close_pd(const char *pkgname, const char *id, int reason);

/**
 * @brief Change the provider control policy.
 * @param[in] ctrl enumeration list - enum PROVIDER_CTRL
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully changed
 */
extern int provider_control(int ctrl);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

/* End of a file */
