/*
 * Copyright 2012  Samsung Electronics Co., Ltd
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

class CVideo {
public:
	enum TVideoState {
		STOPPED = 0x00,
		PLAYING = 0x01,
	};

	static CVideo *Find(enum target_type type, const char *id);
	static CVideo *Create(enum target_type type, const char *id, int w, int h);

	static int s_BufferEventHandler(struct livebox_buffer *buffer, enum buffer_event event, double timestamp, double x, double y, void *data);
	static Eina_Bool s_DamageEventHandler(void *data, int type, void *event);
	static int s_MMMessageHandler(int message, void *_param, void *user_param);

	int Destroy(void);
	int Resize(int w, int h);

	void SetBuffer(struct livebox_buffer *pBuffer);
	enum target_type Type(void) { return m_vType; }
	int Width(void) { return m_nWidth; }
	int Height(void) { return m_nHeight; }
	struct livebox_buffer *Buffer(void) { return m_pBuffer; }
	const char *Id(void) { return m_sId; }
	int PixmapId(void) { return m_nId; }

	int Play(const char *file);
	int Stop(void);
	enum TVideoState State(void);
private:
	CVideo(enum target_type type, int width, int height);
	virtual ~CVideo(void);

	int Update(void);
	void Clear(void);

	enum TVideoState m_vState;
	enum target_type m_vType;
	int m_nWidth;
	int m_nHeight;

	struct livebox_buffer *m_pBuffer;

	int m_nId; /*!< Pixmap ID */
	char *m_sId;
	Ecore_X_Damage m_vDamage;

	char *m_sError;
	MMHandleType m_vPlayer;

	static Eina_List *s_pList;
	Ecore_Event_Handler *m_pDamageHandler;
};
