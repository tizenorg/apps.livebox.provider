/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org
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

/*!
 * Text signal & Content event uses this data structure.
 */
struct event_info {
        struct {
                double x; /*!< X value of current mouse(touch) position */
                double y; /*!< Y value of current mouse(touch) position */
                int down; /*!< Is it pressed(1) or not(0) */
        } pointer;

        struct {
                double sx; /*!< Pressed object's left top X */
                double sy; /*!< Pressed object's left top Y */
                double ex; /*!< Pressed object's right bottom X */
                double ey; /*!< Pressed object's right bottom Y */
        } part;
};

enum access_event {
	ACCESS_HIGHLIGHT,
	ACCESS_HIGHLIGHT_NEXT,
	ACCESS_HIGHLIGHT_PREV,
	ACCESS_ACTIVATE,
	ACCESS_VALUE_CHANGE,
	ACCESS_SCROLL,
	ACCESS_UNHIGHLIGHT,
};

enum access_mouse_state {
	ACCESS_MOUSE_DOWN = 0,
	ACCESS_MOUSE_MOVE = 1,
	ACCESS_MOUSE_UP = 2,
};

struct event_arg {
	enum {
		EVENT_NEW, /*!< Master will send this to create a new livebox instance */
		EVENT_RENEW, /*!< If the master detects any problems of your slave, it will terminate slave provider.
		                  and after reactivating the provider slave, this request will be delievered to create
				  a livebox instance again */

		EVENT_DELETE, /*!< Master will send this to delete a livebox instance */

		EVENT_CONTENT_EVENT, /*!< Any events are generated from your livebox or PD, this event will be sent to you */
		EVENT_CLICKED, /*!< If a livebox is clicked, the master will send this event */
		EVENT_TEXT_SIGNAL, /*!< Text type livebox or PD will generate this event */

		EVENT_RESIZE, /*!< If a livebox is resized, the master will send this event */
		EVENT_SET_PERIOD, /*!< To change the update period of a livebox */
		EVENT_CHANGE_GROUP, /*!< To change the group(cluster/sub-cluster) of a livebox */
		EVENT_PINUP, /*!< To make pin up of a livebox */

		EVENT_UPDATE_CONTENT, /*!< It's time to update the content of a livebox */

		EVENT_PAUSE, /*!< Freeze all timer and go to sleep mode */
		EVENT_RESUME, /*!< Thaw all timer and wake up */

		EVENT_PD_CREATE, /*!< Only for the buffer type */
		EVENT_PD_DESTROY, /*!< Only for the buffer type */
		EVENT_PD_MOVE, /*!< Only for the buffer type */

		EVENT_LB_PAUSE, /*!< Freeze the update timer of a specified livebox */
		EVENT_LB_RESUME, /*!< Thaw the update timer of a specified livebox */

		EVENT_PD_ACCESS, /* PD: Accessibility event */
		EVENT_LB_ACCESS, /* LB: Accessibility event */
	} type;
	const char *pkgname; /*!< Package name of a livebox */
	const char *id; /*!< Instance Id of a livebox */

	union {
		struct {
			int w; /*!< PD buffer is created with width "w" */
			int h; /*!< PD buffer is created with height "h" */

			double x; /*!< Relative X position of a livebox from this PD */
			double y; /*!< Relative Y position of a livebox from this PD */
		} pd_create;

		struct {
		} pd_destroy;

		struct {
			int w; /*!< PD buffer width */
			int h; /*!< PD buffer height */

			double x; /*!< Relative X position of a livebox from this PD */
			double y; /*!< Relative Y position of a livebox from this PD */
		} pd_move;

		struct {
			const char *content; /*!< Content info */
			int timeout; /*!< Timeout */
			int has_script; /*!< Livebox has script (buffer is created from the master) */
			double period; /*!< Update period */
			const char *cluster; /*!< Cluster ID of this livebox instance */
			const char *category; /*!< Sub-cluster ID of this livebox instance */
			int skip_need_to_create; /*!< Is this livebox need to check the "need_to_create"? */
			int width; /*!< Width of a livebox content */
			int height; /*!< Height of a livebox content */
			const char *abi; /*!< ABI tag of this livebox */
			char *out_content; /*!< Output content */
			char *out_title; /*!< Output title */
			int out_is_pinned_up; /*!< Is this pinned up? */
		} lb_create; /*!< "new" */

		struct {
			const char *content;
			int timeout;
			int has_script;
			double period;
			const char *cluster;
			const char *category;
			int width; /*!< Width of a livebox content */
			int height; /*!< Height of a livebox content */
			const char *abi;
			char *out_content; /*!< Output content */
			char *out_title; /*!< Output title */
			int out_is_pinned_up; /*!< Is this pinned up? */
			int hold_scroll; /*!< The scroller which is in viewer is locked */
			int active_update; /*!< Need to know, current update mode */
		} lb_recreate; /*!< renew */

		struct {
		} lb_destroy; /*!< delete */

		struct {
			const char *emission; /*!< Event string */
			const char *source; /*!< Object ID which makes event */
			struct event_info info;
		} content_event; /*!< script */

		struct {
			const char *event; /*!< Event type, currently only "click" supported */
			double timestamp; /*!< Timestamp of event occurred */
			double x; /*!< X position of the click event */
			double y; /*!< Y position of the click event */
		} clicked; /*!< clicked */

		struct {
			const char *emission; /*!< Event string */
			const char *source; /*!< Object ID which makes event */
			struct event_info info;
		} text_signal; /*!< text_signal */

		struct {
			int w; /*!< New width of a livebox */
			int h; /*!< New height of a livebox */
		} resize; /*!< resize */

		struct {
			double period; /*!< New period */
		} set_period; /*!< set_period */

		struct {
			const char *cluster;
			const char *category;
		} change_group; /*!< change_group */

		struct {
			int state;
			char *content_info; /* out value */
		} pinup; /*!< pinup */

		struct {
			const char *cluster;
			const char *category;
		} update_content; /*! update_content */

		struct {
			double timestamp;
		} pause; /*!< pause */

		struct {
			double timestamp;
		} resume; /*!< resume */

		struct {
			/*!< */
		} lb_pause;

		struct {
			/*!< */
		} lb_resume;

		struct {
			/*!< Accessibility */
			enum access_event event;
			int x;
			int y;
			int mouse_state;
		} lb_access;

		struct {
			/*!< Accessibility */
			enum access_event event;
			int x;
			int y;
			int mouse_state;
		} pd_access;

		struct {
			int active_update;
		} update_mode;
	} info;
};

struct event_handler {
	int (*lb_create)(struct event_arg *arg, int *width, int *height, double *priority, void *data); /* new */
	int (*lb_destroy)(struct event_arg *arg, void *data); /* delete */

	/* Recover from the fault of slave */
	int (*lb_recreate)(struct event_arg *arg, void *data);

	int (*lb_pause)(struct event_arg *arg, void *data);
	int (*lb_resume)(struct event_arg *arg, void *data);

	int (*content_event)(struct event_arg *arg, void *data);
	int (*clicked)(struct event_arg *arg, void *data);
	int (*text_signal)(struct event_arg *arg, void *data);
	int (*resize)(struct event_arg *arg, void *data);
	int (*set_period)(struct event_arg *arg, void *data);
	int (*change_group)(struct event_arg *arg, void *data);
	int (*pinup)(struct event_arg *arg, void *data);
	int (*update_content)(struct event_arg *arg, void *data);

	int (*pause)(struct event_arg *arg, void *data);
	int (*resume)(struct event_arg *arg, void *data);

	int (*disconnected)(struct event_arg *arg, void *data);
	int (*connected)(struct event_arg *arg, void *data);

	/*!
	 * \note
	 * Only for the buffer type
	 */
	int (*pd_create)(struct event_arg *arg, void *data);
	int (*pd_destroy)(struct event_arg *arg, void *data);
	int (*pd_move)(struct event_arg *arg, void *data);

	/*!
	 * \note
	 * Accessibility functions
	 */
	int (*lb_access)(struct event_arg *arg, void *data);
	int (*pd_access)(struct event_arg *arg, void *data);

	int (*update_mode)(struct event_arg *arg, void *data);
};

/*!
 * \brief Initialize the provider service
 * \param[in] disp XDisplay object, if you don't know what this is, set NULL
 * \param[in] name Slave name which is given by the master provider.
 * \param[in] table Event handler table
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_init(void *disp, const char *name, struct event_handler *table, void *data);

/*!
 * \brief Finalize the provider service
 */
extern void *provider_fini(void);

/*!
 * \brief Send the hello signal to the master
 *        Master will activate connection of this only if you send this hello event.
 *        or the master will reject all requests from your provider.
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_hello(void);

/*!
 * \brief Send the ping message to the master to notify that your provider is working properly.
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_ping(void);

/*!
 * \brief Send the updated event to the master
 * \param[in] pkgname Package name which of an updated livebox.
 * \param[in] id Instance ID of an updated livebox.
 * \param[in] w Width of an updated content
 * \param[in] h Height of an updated content
 * \param[in] priority Priority of an updated content
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_updated(const char *pkgname, const char *id, int w, int h, double priority, const char *content, const char *title);

/*!
 * \brief Send the description data updated event to the master
 * \param[in] pkgname Package name of an updated livebox.
 * \param[in] id Instance ID of an updated livebox.
 * \param[in] descfile The filename of a description file.
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_desc_updated(const char *pkgname, const char *id, const char *descfile);

/*!
 * \brief
 * \param[in] pkgname
 * \param[in] id
 * \param[in] priority
 * \param[in] content_info
 * \param[in] title
 * \return
 */
extern int provider_send_lb_update_begin(const char *pkgname, const char *id, double priority, const char *content_info, const char *title);

/*!
 * \brief
 * \param[in] pkgname
 * \param[in] id
 * \return
 */
extern int provider_send_lb_update_end(const char *pkgname, const char *id);

/*!
 * \brief
 * \param[in] pkgname
 * \param[in] id
 * \return
 */
extern int provider_send_pd_update_begin(const char *pkgname, const char *id);

/*!
 * \brief
 * \param[in] pkgname
 * \param[in] id
 * \return
 */
extern int provider_send_pd_update_end(const char *pkgname, const char *id);

/*!
 * \brief Send the deleted event of specified livebox instance
 * \param[in] pkgname Package name of the livebox
 * \param[in] id Livebox instance ID
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_deleted(const char *pkgname, const char *id);

/*!
 * \breif If you want to use the fault management service of the master provider,
 *        Before call any functions of a livebox, call this.
 * \param[in] pkgname Package name of the livebox
 * \param[in] id Instance ID of the livebox
 * \param[in] funcname Function name which will be called
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_ret(const char *pkgname, const char *id, const char *funcname);

/*!
 * \brief If you want to use the fault management service of the master provider,
 *        After call any functions of a livebox, call this.
 * \param[in] pkgname Package name of the livebox
 * \param[in] id Instance ID of the livebox
 * \param[in] funcname Function name which is called by the slave
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_call(const char *pkgname, const char *id, const char *funcname);

/*!
 * \brief If you want to send the fault event to the master provider,
 *        Use this API
 * \param[in] pkgname Package name of the livebox
 * \param[in] id ID of the livebox instance
 * \param[in] funcname Reason of the fault error
 * \return int Success LB_STATUS_SUCCESS otherwise errno < 0
 */
extern int provider_send_faulted(const char *pkgname, const char *id, const char *funcname);

/*!
 * \brief If you want notify to viewer to seize the screen,
 *        prevent moving a box from user event
 * \param[in] pkgname Package name of the livebox
 * \param[in] id ID of the livebox instance
 * \param[in] seize 1 if viewer needs to hold a box, or 0
 * \return int Success LB_STATUS_SUCCESS othere LB_STATUS_ERROR_XXX
 */
extern int provider_send_hold_scroll(const char *pkgname, const char *id, int seize);

/*!
 * \brief Notify to viewer for accessiblity event processing status.
 * \param[in] pkgname
 * \param[in] id
 * \param[in] status
 * \return int
 */
extern int provider_send_access_status(const char *pkgname, const char *id, int status);

#ifdef __cplusplus
}
#endif

#endif
/* End of a file */
