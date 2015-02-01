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

extern int util_check_ext(const char *icon, const char *ext);
extern double util_timestamp(void);
extern const char *util_basename(const char *name);
extern char *util_get_current_module(char **symbol);
extern const char *util_uri_to_path(const char *uri);
extern char *util_path_to_uri(const char *id);
extern void *util_timer_add(double interval, Eina_Bool (*cb)(void *data), void *data);
extern void util_timer_interval_set(void *timer, double interval);
extern int util_get_filesize(const char *filename);
extern double util_time_delay_for_compensation(double period);
extern void util_timer_freeze(void *timer);
extern void util_timer_thaw(void *timer);
extern void util_timer_del(void *timer);

extern int util_screen_size_get(int *width, int *height);
extern int util_screen_init(void);
extern int util_screen_fini(void);
extern void *util_screen_get(void);

#define SCHEMA_FILE   "file://"
#define SCHEMA_PIXMAP "pixmap://"
#define SCHEMA_SHM    "shm://"

/* End of a file */
