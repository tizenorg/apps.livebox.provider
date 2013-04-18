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

class CWindow {
public:
	static CWindow *Find(enum target_type type, const char *id);
	static CWindow *Create(enum target_type type, const char *id, int w, int h);

	static void *s_AllocateCanvas(void *data, int size);
	static void s_ReleaseCanvas(void *data, void *canvas);
	static void s_PostRender(void *data, Evas *e, void *event_info);
	static int s_BufferEventHandler(struct livebox_buffer *buffer, enum buffer_event event, double timestamp, double x, double y, void *data);

	int Destroy(void);
	int Resize(int w, int h);

	void SetBuffer(struct livebox_buffer *pBuffer) { m_pBuffer = pBuffer; }
	enum target_type Type(void) { return m_vType; }
	int Width(void) { return m_nWidth; }
	int Height(void) { return m_nHeight; }
	struct livebox_buffer *Buffer(void) { return m_pBuffer; }
	Evas_Object *Object(void) { return m_pObject; }
	const char *Id(void) { return m_sID; }
	Evas *EvasObject(void) { return m_pEvas; }

private:
	CWindow(enum target_type type, int width, int height);
	virtual ~CWindow(void);

	enum target_type m_vType;
	int m_nWidth;
	int m_nHeight;

	struct livebox_buffer *m_pBuffer;

	Ecore_Evas *m_pEcoreEvas;
	Evas *m_pEvas;
	Evas_Object *m_pObject;

	char *m_sID;

	static Eina_List *s_pList;
};

/* End of a file */
