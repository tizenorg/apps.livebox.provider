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

enum buffer_type { /*!< Must have to be sync with libprovider, liblivebox-viewer */
	BUFFER_TYPE_FILE,
	BUFFER_TYPE_SHM,
	BUFFER_TYPE_PIXMAP,
	BUFFER_TYPE_ERROR
};

struct livebox_buffer {
	enum {
		BUFFER_INITIALIZED = 0x0b0e0e0f,
		BUFFER_CREATED = 0x00beef00,
		BUFFER_DESTROYED = 0x00dead00,
	} state;

	enum target_type type;

	union {
		int fd; /* File handle(descriptor) */
		int id; /* SHM handle(id) */
	} handle;

	char *pkgname;
	char *id;
	int width;
	int height;
	int pixel_size;

	struct fb_info *fb;

	int (*handler)(struct livebox_buffer *info, enum buffer_event event, double timestamp, double x, double y, void *data);
	void *data;

	/* Add to the end of this structure */
	int (*handler_NEW)(struct livebox_buffer *info, struct buffer_event_data *event_info, void *data);
	void *user_data;
	char *lock;
	int lock_fd;
};

extern struct livebox_buffer *provider_buffer_find_buffer(enum target_type type, const char *pkgname, const char *id);

extern struct packet *provider_buffer_lb_mouse_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_move(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_enter(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_leave(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_on_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_off_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_on_hold(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_mouse_off_hold(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_key_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_key_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_key_focus_out(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_key_focus_in(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_mouse_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_move(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_enter(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_leave(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_on_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_off_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_on_hold(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_mouse_off_hold(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_key_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_key_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_key_focus_out(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_key_focus_in(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_action_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_action_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_scroll_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_scroll_move(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_scroll_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_unhighlight(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_hl(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_hl_prev(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_hl_next(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_pd_access_activate(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_unhighlight(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_hl(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_hl_prev(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_hl_next(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_action_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_action_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_scroll_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_scroll_move(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_scroll_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_lb_access_activate(pid_t pid, int handle, const struct packet *packet);

extern const char *provider_name(void);

/* End of a file */
