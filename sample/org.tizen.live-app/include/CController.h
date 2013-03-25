/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class CController {
public:
	static int Initialize(const char *name);
	static int Finalize(void);

	static int s_CreateLB(struct event_arg *arg, int *width, int *height, double *priority, void *data);
	static int s_RecreateLB(struct event_arg *arg, void *data);
	static int s_DestroyLB(struct event_arg *arg, void *data);
	static int s_ContentEvent(struct event_arg *arg, void *data);
	static int s_Clicked(struct event_arg *arg, void *data);
	static int s_TextEvent(struct event_arg *arg, void *data);
	static int s_Resize(struct event_arg *arg, void *data);
	static int s_SetPeriod(struct event_arg *arg, void *data);
	static int s_ChangeGroup(struct event_arg *arg, void *data);
	static int s_Pinup(struct event_arg *arg, void *data);
	static int s_UpdateContent(struct event_arg *arg, void *data);
	static int s_Pause(struct event_arg *arg, void *data);
	static int s_Resume(struct event_arg *arg, void *data);
	static int s_Disconnected(struct event_arg *arg, void *data);
	static int s_Connected(struct event_arg *arg, void *data);
	static int s_CreatePD(struct event_arg *arg, void *data);
	static int s_DestroyPD(struct event_arg *arg, void *data);
	static Eina_Bool s_PingHandler(void *data);

private:
	CController(void);
	virtual ~CController(void);

	char *m_sName;

	static CController *m_pInstance;
	static Ecore_Timer *m_pPing;

	static const double m_nPingInterval;
};

/* End of a file */
