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

#include <sys/types.h>
#include <dynamicbox_buffer.h>

extern int provider_buffer_direct_key_down(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_key_up(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_key_focus_in(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_key_focus_out(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_mouse_enter(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_mouse_leave(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_mouse_down(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_mouse_up(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);
extern int provider_buffer_direct_mouse_move(dynamicbox_buffer_h info, dynamicbox_buffer_event_data_t data);

extern struct packet *provider_buffer_dbox_mouse_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_move(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_enter(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_leave(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_on_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_off_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_on_hold(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_mouse_off_hold(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_dbox_key_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_key_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_key_focus_out(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_key_focus_in(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_gbar_mouse_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_move(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_enter(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_leave(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_on_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_off_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_on_hold(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_mouse_off_hold(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_gbar_key_down(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_key_up(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_key_focus_out(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_key_focus_in(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_gbar_access_action(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_hl(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_activate(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_value_change(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_mouse(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_back(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_over(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_read(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_gbar_access_enable(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_dbox_access_hl(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_action(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_scroll(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_activate(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_value_change(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_mouse(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_back(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_over(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_read(pid_t pid, int handle, const struct packet *packet);
extern struct packet *provider_buffer_dbox_access_enable(pid_t pid, int handle, const struct packet *packet);

extern const char *provider_name(void);

/* End of a file */
