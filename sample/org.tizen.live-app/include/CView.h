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

class CView {
public:
	/*! \TODO: fill me */
	static CView *Create(CWindow *win);
	int Destroy(void);

	void ResetLayout(void);

private:
	CView(void);
	virtual ~CView(void);

	int InitLayout(void);

	CWindow *m_pWindow;
	Evas_Object *m_pScroller;
};

/* End of a file */
